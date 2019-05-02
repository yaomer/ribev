#ifndef _HTTP_HTTP_H
#define _HTTP_HTTP_H

enum {
    HTTP_LINE,
    HTTP_HEADER,
    HTTP_OK,
    HTTP_ERROR,
};

typedef struct {
    int status;
    char method[8];
    char url[255];
    char version[8];
    char host[255];
} HTTP_REQUEST;

void http_parse_line(rb_channel_t *chl, size_t len);
void http_parse_header(rb_channel_t *chl, size_t len);
void http_response(rb_channel_t *chl);

#endif /* _HTTP_HTTP_H */
