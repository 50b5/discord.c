#include "http.h"

#include "c-utils/log.h"
#include "c-utils/str.h"

#include <stdlib.h>
#include <time.h>

#include <curl/curl.h>

static const logctx *logger = NULL;

struct responsestr {
    char *data;
    size_t length;
};

static bool is_rate_limited(discord_http *http, const char *bucket){
    if (!http){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] get_rate_limit() - http is NULL\n",
            __FILE__
        );

        return true;
    }
    else if (!bucket){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] is_rate_limited() - bucket is NULL\n",
            __FILE__
        );

        return true;
    }

    if (http->globalratelimit){
        bucket = "global";
    }

    size_t bucketlen = strlen(bucket);
    double retryafter = map_get_double(http->buckets, bucketlen, bucket);

    if (retryafter > 0){
        time_t curr = time(NULL);
        time_t diff = retryafter - curr;

        if (retryafter >= curr){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] is_rate_limited() - rate limit expires in %lu seconds (bucket: %s)\n",
                __FILE__,
                diff,
                bucket
            );

            return true;
        }

        map_remove(http->buckets, bucketlen, bucket);
    }

    http->ratelimited = false;
    http->globalratelimit = false;

    return false;
}

static bool set_rate_limited(discord_http *http, const char *bucket, double retryafter){
    if (!http){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_rate_limited() - http is NULL\n",
            __FILE__
        );

        return false;
    }

    if (http->globalratelimit){
        bucket = "global";
    }

    map_item k = {0};
    k.type = M_TYPE_STRING;
    k.size = strlen(bucket);
    k.data_copy = bucket;

    map_item v = {0};
    v.type = M_TYPE_DOUBLE;
    v.size = sizeof(retryafter);
    v.data_copy = &retryafter;

    bool success = map_set(http->buckets, &k, &v);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_rate_limited() - failed to store bucket rate limit\n",
            __FILE__
        );
    }

    http->ratelimited = true;

    return success;
}

static size_t write_response_headers(char *data, size_t size, size_t nitems, void *out){
    size *= nitems;
    map *m = out;

    list *parts = string_split_len(data, size, ": ", 1);

    if (!parts){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] write_response_headers() - string_split_len call failed\n",
            __FILE__
        );

        return 0;
    }
    else if (list_get_length(parts) != 2){
        list_free(parts);

        return size;
    }

    const char *key = list_get_string(parts, 0);
    const char *value = list_get_string(parts, 1);

    map_item k = {0};
    k.type = M_TYPE_STRING;
    k.size = strlen(key);
    k.data_copy = key;

    map_item v = {0};
    v.type = M_TYPE_STRING;
    v.size = strlen(value) - 2;
    v.data_copy = value;

    bool success = map_set(m, &k, &v);

    list_free(parts);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] write_response_headers() - map_set call failed\n",
            __FILE__
        );

        return 0;
    }

    return size;
}

static size_t write_response_data(char *data, size_t length, size_t nmemb, void *out){
    length *= nmemb;
    struct responsestr *res = out;

    char *tmp = realloc(res->data, res->length + length + 1);

    if (!tmp){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] write_response_data() - buffer data realloc failed\n",
            __FILE__
        );

        return 0;
    }

    memcpy(tmp + res->length, data, length);

    res->data = tmp;
    res->length += length;
    res->data[res->length] = '\0';

    return length;
}

static char *create_request_bucket(const char *path){
    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_bucket() - path is NULL\n",
            __FILE__
        );

        return NULL;
    }

    list *parts = string_split(path, "/", -1);

    if (!parts){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_bucket() - failed to split path string\n",
            __FILE__
        );

        return NULL;
    }

    const char *channelid = "NULL";
    const char *guildid = "NULL";

    if (!strcmp(list_get_string(parts, 1), "channels")){
        channelid = list_get_string(parts, 2);
    }
    else if (!strcmp(list_get_string(parts, 1), "guilds")){
        guildid = list_get_string(parts, 2);
    }

    char *bucket = string_create("%s:%s:%s", channelid, guildid, path);

    list_free(parts);

    if (!bucket){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_bucket() - failed to create bucket string\n",
            __FILE__
        );

        return NULL;
    }

    return bucket;
}

static struct curl_slist *create_request_header_list(const discord_http *http, const discord_http_request_options *opts){
    if (!http){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_header_list() - http is NULL\n",
            __FILE__
        );

        return NULL;
    }

    struct curl_slist *headers = NULL;
    struct curl_slist *tmp = NULL;

    if (opts){
        if (opts->data){
            tmp = curl_slist_append(NULL, "Content-Type: application/json");

            if (!tmp){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] create_request_header_list() - failed to append content type header\n",
                    __FILE__
                );

                return NULL;
            }

            headers = tmp;
        }

        if (opts->reason){
            char *reason = string_create("X-Audit-Log-Reason: %s", opts->reason);

            if (!reason){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] create_request_header_list() - failed to create reason string\n",
                    __FILE__
                );

                curl_slist_free_all(headers);

                return NULL;
            }

            tmp = curl_slist_append(headers, reason);

            free(reason);

            if (!tmp){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] create_request_header_list() - failed to append reason header\n",
                    __FILE__
                );

                curl_slist_free_all(headers);

                return NULL;
            }

            headers = tmp;
        }
    }

    char *authorization = string_create("Authorization: Bot %s", http->token);

    if (!authorization){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_header_list() - failed to create authorization string\n",
            __FILE__
        );

        curl_slist_free_all(headers);

        return NULL;
    }

    tmp = curl_slist_append(headers, authorization);

    free(authorization);

    if (!tmp){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_header_list() - failed to append authorization header\n",
            __FILE__
        );

        curl_slist_free_all(headers);

        return NULL;
    }

    headers = tmp;

    char *useragent = string_create("User-Agent: %s", DISCORD_USER_AGENT);

    if (!useragent){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_header_list() - failed to create user agent string\n",
            __FILE__
        );

        curl_slist_free_all(headers);

        return NULL;
    }

    tmp = curl_slist_append(headers, useragent);

    free(useragent);

    if (!tmp){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_header_list() - failed to append user agent header\n",
            __FILE__
        );

        curl_slist_free_all(headers);

        return NULL;
    }

    headers = tmp;

    tmp = curl_slist_append(headers, "Accept: application/json");

    if (!tmp){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_request_header_list() - failed to append accept header\n",
            __FILE__
        );

        curl_slist_free_all(headers);

        return NULL;
    }

    headers = tmp;

    return headers;
}

static bool set_request_method(CURL *handle, discord_http_method method, const discord_http_request_options *opts){
    if (!handle){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_request_method() - handle is NULL\n",
            __FILE__
        );

        return false;
    }

    CURLcode err = CURLE_FAILED_INIT;

    if (!opts || !opts->data){
        err = curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, 0);

        if (err != CURLE_OK){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_request_method() - set CURLOPT_POSTFIELDSIZE failed\n",
                __FILE__
            );

            return false;
        }
    }
    else if (opts->data){
        const char *jsonstr = json_object_to_json_string(opts->data);

        if (!jsonstr){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_request_method() - failed to get json_object string\n",
                __FILE__
            );

            return false;
        }

        err = curl_easy_setopt(handle, CURLOPT_POSTFIELDS, jsonstr);

        if (err != CURLE_OK){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_request_method() - failed to set CURLOPT_POSTFIELDS\n",
                __FILE__
            );

            return false;
        }
    }

    switch (method){
    case DISCORD_HTTP_GET:
        err = curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);

        if (err != CURLE_OK){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_request_method() - failed to set CURLOPT_HTTPGET\n",
                __FILE__
            );

            return false;
        }

        break;
    case DISCORD_HTTP_DELETE:
        err = curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE");

        if (err != CURLE_OK){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_request_method() - failed to set CURLOPT_CUSTOMREQUEST (DELETE)\n",
                __FILE__
            );

            return false;
        }

        break;
    case DISCORD_HTTP_PATCH:
        err = curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PATCH");

        if (err != CURLE_OK){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_request_method() - failed to set CURLOPT_CUSTOMREQUEST (PATCH)\n",
                __FILE__
            );

            return false;
        }

        break;
    case DISCORD_HTTP_POST:
        err = curl_easy_setopt(handle, CURLOPT_POST, 1);

        if (err != CURLE_OK){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_request_method() - failed to set CURLOPT_POST\n",
                __FILE__
            );

            return false;
        }

        break;
    case DISCORD_HTTP_PUT:
        err = curl_easy_setopt(handle, CURLOPT_UPLOAD, 1);

        if (err != CURLE_OK){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_request_method() - failed to set CURLOPT_UPLOAD\n",
                __FILE__
            );

            return false;
        }

        break;
    default:
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_request_method() - unimplemented request method\n",
            __FILE__
        );

        return false;
    }

    return true;
}

static bool set_request_url(CURL *handle, const char *path){
    if (!handle){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_request_url() - handle is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_request_url() - path is NULL\n",
            __FILE__);

        return false;
    }

    char *url = string_create("%s%s", DISCORD_API, path);

    if (!url){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_request_url() - failed to create URL string\n",
            __FILE__);

        return false;
    }

    CURLcode err = curl_easy_setopt(handle, CURLOPT_URL, url);

    free(url);

    if (err != CURLE_OK){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_request_url() - failed to set CURLOPT_URL\n",
            __FILE__
        );

        return false;
    }

    return true;
}

static bool set_request_headers(CURL *handle, const struct curl_slist *headers){
    if (!handle){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_request_headers() - handle is NULL\n",
            __FILE__
        );

        return false;
    }

    CURLcode err = curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    if (err != CURLE_OK){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_request_headers() - failed to set CURLOPT_HTTPHEADER\n",
            __FILE__
        );

        return false;
    }

    return true;
}

static bool set_response_header_writer(CURL *handle, void *out){
    if (!handle){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_response_header_writer() - handle is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!out){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_response_header_writer() - out is NULL\n",
            __FILE__
        );

        return false;
    }

    CURLcode err = curl_easy_setopt(
        handle,
        CURLOPT_HEADERFUNCTION,
        write_response_headers
    );

    if (err != CURLE_OK){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_response_header_writer() - failed to set CURLOPT_HEADERFUNCTION\n",
            __FILE__
        );

        return false;
    }

    err = curl_easy_setopt(handle, CURLOPT_HEADERDATA, out);

    if (err != CURLE_OK){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_response_header_writer() - failed to set CURLOPT_HEADERDATA\n",
            __FILE__
        );

        return false;
    }

    return true;
}

static bool set_response_data_writer(CURL *handle, void *out){
    if (!handle){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_response_data_writer() - handle is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!out){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_response_data_writer() - out is NULL\n",
            __FILE__
        );

        return false;
    }

    CURLcode err = curl_easy_setopt(
        handle,
        CURLOPT_WRITEFUNCTION,
        write_response_data
    );

    if (err != CURLE_OK){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_response_data_writer() - failed to set CURLOPT_WRITEFUNCTION\n",
            __FILE__
        );

        return false;
    }

    err = curl_easy_setopt(handle, CURLOPT_WRITEDATA, out);

    if (err != CURLE_OK){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_response_data_writer() - failed to set CURLOPT_WRITEDATA\n",
            __FILE__
        );

        return false;
    }

    return true;
}

static discord_http_response *create_response(CURL *handle, map *headers){
    if (!handle){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_response() - handle is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = calloc(1, sizeof(*response));

    if (!response){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_response() - calloc for response failed\n",
            __FILE__
        );

        return NULL;
    }

    CURLcode err = curl_easy_getinfo(
        handle,
        CURLINFO_RESPONSE_CODE,
        &response->status
    );

    if (err != CURLE_OK){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] create_response() - failed to get CURLINFO_RESPONSE_CODE\n",
            __FILE__
        );

        free(response);

        return NULL;
    }

    response->headers = headers;

    return response;
}

static void handle_response_status(discord_http *http, const char *bucket, const discord_http_response *response){
    if (!http){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] handle_response_status() - http is NULL\n",
            __FILE__
        );

        return;
    }
    else if (!response){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] handle_response_status() - response is NULL\n",
            __FILE__
        );

        return;
    }

    bool success = false;
    double retryafter = 0;
    json_object *obj = NULL;

    switch (response->status){
    case 200:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) request completed successfully",
            __FILE__,
            response->status
        );

        break;
    case 201:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) entity created successfully",
            __FILE__,
            response->status
        );

        break;
    case 204:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) request completed successfully but returned no content",
            __FILE__,
            response->status
        );

        break;
    case 304:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) resource was not modified",
            __FILE__,
            response->status
        );

        break;
    case 400:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) sent invalid request",
            __FILE__,
            response->status
        );

        break;
    case 401:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) sent unauthorized request",
            __FILE__,
            response->status
        );

        break;
    case 403:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) requested privileged resource",
            __FILE__,
            response->status
        );

        break;
    case 404:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) requested nonexistent resource",
            __FILE__,
            response->status
        );

        break;
    case 405:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) invalid method for requested resource",
            __FILE__,
            response->status
        );

        break;
    case 429:
        obj = json_object_object_get(response->data, "retry_after");

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_response_status() - failed to get retry_after json object",
                __FILE__
            );

            break;
        }

        retryafter = json_object_get_double(obj);
        obj = json_object_object_get(response->data, "global");

        if (obj && json_object_get_boolean(obj)){
            http->globalratelimit = true;
        }

        success = set_rate_limited(http, bucket, time(NULL) + retryafter);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_response_status() - failed to set rate limit (bucket: %s)",
                __FILE__,
                bucket
            );

            break;
        }

        log_write(
            logger,
            LOG_WARNING,
            "[%s] handle_response_status() - rate limit exceeded (bucket: %s)",
            __FILE__,
            bucket
        );

        break;
    default:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_response_status() - (%d) unexpected status code",
            __FILE__,
            response->status
        );
    }

    if (response->data){
        json_object *message = json_object_object_get(
            response->data,
            "message"
        );

        if (message){
            log_write(
                logger,
                LOG_RAW,
                " -- (Error: %s)",
                json_object_get_string(message)
            );
        }
    }

    log_write(logger, LOG_RAW, "\n");
}

discord_http *discord_http_init(const char *token, const discord_http_options *opts){
    if (!opts){
        /* unused for now */
    }

    if (!token){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_init() - token is NULL, refusing to initialize\n",
            __FILE__
        );

        return NULL;
    }

    if (curl_global_init(CURL_GLOBAL_ALL)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_init() - curl_global_init call failed\n",
            __FILE__
        );

        return NULL;
    }

    map *buckets = map_init();

    if (!buckets){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_init() - failed to initialize rate limit buckets\n",
            __FILE__
        );

        curl_global_cleanup();

        return NULL;
    }

    discord_http *http = calloc(1, sizeof(*http));

    if (!http){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_init() - alloc for discord_http failed\n",
            __FILE__
        );

        curl_global_cleanup();
        map_free(buckets);

        return NULL;
    }

    http->token = token;
    http->buckets = buckets;

    return http;
}

discord_http_response *discord_http_request(discord_http *http, discord_http_method method, const char *path, const discord_http_request_options *opts){
    if (!http){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - http is NULL\n",
            __FILE__
        );

        return NULL;
    }
    else if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - path is NULL\n",
            __FILE__
        );

        return NULL;
    }

    char *bucket = create_request_bucket(path);

    if (!bucket){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - create_request_bucket call failed\n",
            __FILE__
        );

        return NULL;
    }

    if (is_rate_limited(http, bucket)){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] discord_http_request() - refusing request, rate limited (bucket: %s)\n",
            __FILE__,
            (http->globalratelimit ? "global" : bucket)
        );

        free(bucket);

        return NULL;
    }

    CURL *handle = curl_easy_init();

    if (!handle){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - curl_easy_init call failed\n"
            __FILE__
        );

        free(bucket);

        return NULL;
    }

    if (!set_request_method(handle, method, opts)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - set_request_method call failed\n",
            __FILE__
        );

        curl_easy_cleanup(handle);
        free(bucket);

        return NULL;
    }

    if (!set_request_url(handle, path)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - set_request_url call failed\n",
            __FILE__
        );

        curl_easy_cleanup(handle);
        free(bucket);

        return NULL;
    }

    struct curl_slist *requestheaders = create_request_header_list(http, opts);

    if (!requestheaders){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - create_request_header_list call failed\n",
            __FILE__
        );

        curl_easy_cleanup(handle);
        free(bucket);

        return NULL;
    }

    if (!set_request_headers(handle, requestheaders)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - set_request_headers call failed\n",
            __FILE__
        );

        curl_slist_free_all(requestheaders);
        curl_easy_cleanup(handle);
        free(bucket);

        return NULL;
    }

    struct responsestr out = {0};

    if (!set_response_data_writer(handle, &out)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - set_response_data_writer call failed\n",
            __FILE__
        );

        curl_slist_free_all(requestheaders);
        curl_easy_cleanup(handle);
        free(bucket);

        return NULL;
    }

    map *responseheaders = map_init();

    if (!responseheaders){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - map_init call failed\n",
            __FILE__
        );

        curl_slist_free_all(requestheaders);
        curl_easy_cleanup(handle);
        free(bucket);

        return NULL;
    }

    if (!set_response_header_writer(handle, responseheaders)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - set_response_header_writer call failed\n",
            __FILE__
        );

        map_free(responseheaders);
        curl_slist_free_all(requestheaders);
        curl_easy_cleanup(handle);
        free(bucket);

        return NULL;
    }

    CURLcode err = curl_easy_perform(handle);

    curl_slist_free_all(requestheaders);

    if (err != CURLE_OK){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - failed to perform request: %s\n",
            __FILE__,
            curl_easy_strerror(err)
        );

        map_free(responseheaders);
        curl_easy_cleanup(handle);
        free(bucket);

        return NULL;
    }

    discord_http_response *response = create_response(handle, responseheaders);

    curl_easy_cleanup(handle);

    if (!response){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_http_request() - create_response call failed\n",
            __FILE__
        );

        map_free(responseheaders);
        free(bucket);
    }

    if (out.length > 0){
        response->data = json_tokener_parse(out.data);

        free(out.data);

        if (!response->data){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] discord_http_request() - json_tokener_parse call failed\n",
                __FILE__
            );
        }
    }

    handle_response_status(http, bucket, response);

    free(bucket);

    return response;
}

discord_http_response *discord_http_get_gateway(discord_http *http){
    return discord_http_request(http, DISCORD_HTTP_GET, "/gateway", NULL);
}

discord_http_response *discord_http_get_bot_gateway(discord_http *http){
    return discord_http_request(http, DISCORD_HTTP_GET, "/gateway/bot", NULL);
}

discord_http_response *discord_http_get_current_application_information(discord_http *http){
    return discord_http_request(http, DISCORD_HTTP_GET, "/oauth2/applications/@me", NULL);
}

/* User */
discord_http_response *discord_http_get_current_user(discord_http *http){
    return discord_http_request(http, DISCORD_HTTP_GET, "/users/@me", NULL);
}

discord_http_response *discord_http_get_user(discord_http *http, snowflake userid){
    char *path = string_create(
        "/users/%" PRIu64,
        userid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_user() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(http, DISCORD_HTTP_GET, path, NULL);

    free(path);

    return response;
}

discord_http_response *discord_http_modify_current_user(discord_http *http, const char *username, const char *avatar){
    json_object *data = json_object_new_object();

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_modify_current_user() - data object initialization failed\n",
            __FILE__
        );

        return NULL;
    }

    json_object *usernameobj = json_object_new_string(username);
    json_object *avatarobj = json_object_new_string(avatar);

    json_object_object_add(data, "username", usernameobj);
    json_object_object_add(data, "avatar", avatarobj);

    discord_http_request_options opts = {0};
    opts.data = data;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_PATCH,
        "/users/@me",
        &opts
    );

    json_object_put(data);

    return response;
}

discord_http_response *discord_http_get_current_user_guilds(discord_http *http, snowflake before, snowflake after, int limit){
    json_object *data = json_object_new_object();

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_current_user_guilds() - data object initialization failed\n",
            __FILE__
        );

        return NULL;
    }

    if (before){
        json_object *beforeobj = json_object_new_int64(before);

        if (!beforeobj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] http_get_current_user_guilds() - before object initialization failed\n",
                __FILE__
            );

            json_object_put(data);

            return NULL;
        }

        json_object_object_add(data, "before", beforeobj);
    }

    if (after){
        json_object *afterobj = json_object_new_int64(after);

        if (!afterobj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] http_get_current_user_guilds() - after object initialization failed\n",
                __FILE__
            );

            json_object_put(data);

            return NULL;
        }

        json_object_object_add(data, "after", afterobj);
    }

    if (limit){
        json_object *limitobj = json_object_new_int(limit);

        if (!limitobj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] http_get_current_user_guilds() - limit object initialization failed\n",
                __FILE__
            );

            json_object_put(data);

            return NULL;
        }

        json_object_object_add(data, "limit", limitobj);
    }

    discord_http_request_options opts = {0};
    opts.data = data;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_PATCH,
        "/users/@me/guilds",
        &opts
    );

    json_object_put(data);

    return response;
}

discord_http_response *discord_http_get_current_user_guild_member(discord_http *http, snowflake guildid){
    char *path = string_create(
        "/users/@me/guilds/%" PRIu64 "/member",
        guildid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_current_user_guild_member() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_GET,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_leave_guild(discord_http *http, snowflake guildid){
    char *path = string_create(
        "/users/@me/guilds/%" PRIu64,
        guildid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_leave_guild() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_create_dm(discord_http *http, snowflake recipientid){
    json_object *data = json_object_new_object();

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_create_dm() - data object initialization failed\n",
            __FILE__
        );

        return NULL;
    }

    json_object *recipientidobj = json_object_new_int64(recipientid);

    if (!recipientidobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_create_dm() - recipientid object initialization failed\n",
            __FILE__
        );

        json_object_put(data);

        return NULL;
    }

    json_object_object_add(data, "recipient_id", recipientidobj);

    discord_http_request_options opts = {0};
    opts.data = data;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_POST,
        "/users/@me/channels",
        &opts
    );

    json_object_put(data);

    return response;
}

/*
discord_http_response *discord_http_create_group_dm(discord_http *http, const json_object *data){

}
*/

discord_http_response *discord_http_get_user_connections(discord_http *http){
    return discord_http_request(http, DISCORD_HTTP_GET, "/users/@me/connections", NULL);
}

/* Channel */
discord_http_response *discord_http_trigger_typing_indicator(discord_http *http, snowflake channelid){
    char *path = string_create(
        "/channels/%" PRIu64 "/typing",
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_trigger_typing_indicator() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_POST,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_follow_news_channel(discord_http *http, snowflake channelid, json_object *data){
    char *path = string_create(
        "/channels/%" PRIu64 "/followers",
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_follow_news_channel() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.data = data;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_POST,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_get_channel(discord_http *http, snowflake channelid){
    char *path = string_create(
        "/channels/%" PRIu64,
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_channel() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_GET,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_edit_channel(discord_http *http, snowflake channelid, json_object *data, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64,
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_edit_channel() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.data = data;
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_PATCH,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_delete_channel(discord_http *http, snowflake channelid, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64,
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_delete_channel() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_edit_channel_permissions(discord_http *http, snowflake channelid, snowflake overwriteid, json_object *data, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64 "/permissions/%" PRIu64,
        channelid,
        overwriteid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_edit_channel_permissions() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.data = data;
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_PUT,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_delete_channel_permission(discord_http *http, snowflake channelid, snowflake overwriteid, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64 "/permissions/%" PRIu64,
        channelid,
        overwriteid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_delete_channel_permission() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_get_channel_invites(discord_http *http, snowflake channelid){
    char *path = string_create(
        "/channels/%" PRIu64 "/invites",
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_channel_invites() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_GET,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_create_channel_invite(discord_http *http, snowflake channelid, json_object *data, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64 "/invites",
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_create_channel_invite() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.data = data;
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_POST,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_get_pinned_messages(discord_http *http, snowflake channelid){
    char *path = string_create(
        "/channels/%" PRIu64 "/pins",
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_pinned_messages() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_GET,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_pin_message(discord_http *http, snowflake channelid, snowflake messageid, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64 "/pins/%" PRIu64,
        channelid,
        messageid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_pin_message() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_PUT,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_unpin_message(discord_http *http, snowflake channelid, snowflake messageid, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64 "/pins/%" PRIu64,
        channelid,
        messageid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_unpin_message() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        &opts
    );

    free(path);

    return response;
}

/* Message */
discord_http_response *discord_http_get_channel_messages(discord_http *http, snowflake channelid, json_object *data){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages",
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_channel_messages() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.data = data;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_GET,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_get_channel_message(discord_http *http, snowflake channelid, snowflake messageid){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64,
        channelid,
        messageid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_channel_message() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_GET,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_crosspost_message(discord_http *http, snowflake channelid, snowflake messageid){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64 "/crosspost",
        channelid,
        messageid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_crosspost_message() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_POST,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_create_message(discord_http *http, snowflake channelid, json_object *data){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages",
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_create_message() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.data = data;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_POST,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_edit_message(discord_http *http, snowflake channelid, snowflake messageid, json_object *data){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64,
        channelid,
        messageid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_edit_message() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.data = data;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_PATCH,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_delete_message(discord_http *http, snowflake channelid, snowflake messageid, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64,
        channelid,
        messageid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_delete_message() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_bulk_delete_messages(discord_http *http, snowflake channelid, json_object *data, const char *reason){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/bulk-delete",
        channelid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_bulk_delete_messages() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_request_options opts = {0};
    opts.data = data;
    opts.reason = reason;

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_POST,
        path,
        &opts
    );

    free(path);

    return response;
}

discord_http_response *discord_http_get_reactions(discord_http *http, snowflake channelid, snowflake messageid, const char *emoji){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64 "/reactions/%s",
        channelid,
        messageid,
        emoji
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_get_reactions() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_GET,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_create_reaction(discord_http *http, snowflake channelid, snowflake messageid, const char *emoji){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64 "/reactions/%s",
        channelid,
        messageid,
        emoji
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_create_reaction() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_PUT,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_delete_own_reaction(discord_http *http, snowflake channelid, snowflake messageid, const char *emoji){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64 "/reactions/%s/@me",
        channelid,
        messageid,
        emoji
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_delete_own_reaction() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_delete_user_reaction(discord_http *http, snowflake channelid, snowflake messageid, snowflake userid, const char *emoji){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64 "/reactions/%s/%" PRIu64,
        channelid,
        messageid,
        emoji,
        userid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_delete_user_reaction() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_delete_all_reactions(discord_http *http, snowflake channelid, snowflake messageid){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64 "/reactions",
        channelid,
        messageid
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_delete_all_reactions() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        NULL
    );

    free(path);

    return response;
}

discord_http_response *discord_http_delete_all_reactions_for_emoji(discord_http *http, snowflake channelid, snowflake messageid, const char *emoji){
    char *path = string_create(
        "/channels/%" PRIu64 "/messages/%" PRIu64 "/reactions/%s",
        channelid,
        messageid,
        emoji
    );

    if (!path){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] http_delete_all_reactions_for_emoji() - string_create call failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_http_response *response = discord_http_request(
        http,
        DISCORD_HTTP_DELETE,
        path,
        NULL
    );

    free(path);

    return response;
}

void discord_http_response_free(discord_http_response *response){
    if (!response){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] http_response_free() - response is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(response->data);
    map_free(response->headers);
    free(response);
}

void discord_http_free(discord_http *http){
    if (!http){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] http_free() - http is NULL\n",
            __FILE__
        );

        return;
    }

    map_free(http->buckets);
    free(http);

    curl_global_cleanup();
}
