#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_


typedef enum
{
    HTTP_GET_TIME = 0,
    HTTP_GET_WEATHER,
    HTTP_GET_FANS,
    HTTP_GET_CITY,
} HTTP_GET_TYPE_E;

typedef struct
{
    HTTP_GET_TYPE_E type;
} HTTP_GET_EVENT_T;

// void http_request_init();
void http_time_get();
// void http_set_type(HTTP_GET_TYPE_E type);

#endif
