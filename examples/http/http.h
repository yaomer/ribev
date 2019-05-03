#ifndef _HTTP_HTTP_H
#define _HTTP_HTTP_H

enum {
    HTTP_LINE,
    HTTP_HEADER,
    HTTP_OK,
    HTTP_ERROR,
};

#define HTTP_ALIVE 001

typedef struct {
    int status;
    int flag;
    char method[8];
    char url[255];
    char version[12];
    char host[255];
    char conn[32];
} HTTP_REQUEST;

void http_parse_line(rb_channel_t *chl, size_t len);
void http_parse_header(rb_channel_t *chl, size_t len);
void http_response(rb_channel_t *chl);

#endif /* _HTTP_HTTP_H */
