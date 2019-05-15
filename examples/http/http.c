#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../usr_serv.h"
#include "http.h"

static void
http_header(rb_buffer_t *buf)
{
    char *s = "Ribev-Server\r\n";
    rb_buffer_write(buf, s, strlen(s));
    s = "Content-Type: text/html\r\n";
    rb_buffer_write(buf, s, strlen(s));
    s = "\r\n";
    rb_buffer_write(buf, s, strlen(s));
}

static void
http_send(rb_channel_t *chl, rb_buffer_t *buf)
{
    char *s = rb_buffer_begin(buf);
    size_t len = rb_buffer_readable(buf);
    printf("send %zu\n", len);
    rb_send(chl, s, len);
    rb_buffer_retrieve(buf, len);
}

static void
http_ok(rb_channel_t *chl, rb_buffer_t *buf)
{
    char *s = "HTTP/1.1 200 OK\r\n";
    rb_buffer_write(buf, s, strlen(s));
    http_header(buf);
    http_send(chl, buf);
}

static void
http_not_found(rb_channel_t *chl, rb_buffer_t *buf)
{
    char *s = "HTTP/1.1 404 NOT FOUND\r\n";
    rb_buffer_write(buf, s, strlen(s));
    http_header(buf);
    http_send(chl, buf);
}

void
http_unimplemented(rb_channel_t *chl, rb_buffer_t *buf)
{
    char *s = "HTTP/1.1 501 METHOD NOT IMPLEMENTED\r\n";
    rb_buffer_write(buf, s, strlen(s));
    http_header(buf);
    http_send(chl, buf);
}

/*
 * 解析请求行
 */
void
http_parse_line(rb_channel_t *chl, size_t len)
{
    HTTP_REQUEST *req = (HTTP_REQUEST *)chl->user.data;
    char *p = rb_buffer_begin(chl->input);
    char *ep = p + len;
    struct stat st;

    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;
    /* 请求方法 */
    int i = 0;
    int j = sizeof(req->method) - 1;
    while (p < ep && i < j && !isspace(*p))
        req->method[i++] = *p++;
    req->method[i] = '\0';
    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;
    /* url */
    i = 0;
    j = sizeof(req->url) - 1;
    req->url[i++] = '.';
    while (p < ep && i < j && !isspace(*p))
        req->url[i++] = *p++;
    req->url[i] = '\0';
    if (strcmp(req->url, "./") == 0)
        strcat(req->url, "index.html");
    else if (stat(req->url, &st) < 0) {
        req->status = HTTP_ERROR;
        http_response(chl);
        return;
    } else {
        if (S_ISDIR(st.st_mode))
            strcat(req->url, "/index.html");
    }
    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;
    /* version */
    i = 0;
    j = sizeof(req->version) - 1;
    while (p < ep && i < j && !isspace(*p))
        req->version[i++] = *p++;
    req->version[i] = '\0';
    req->status = HTTP_HEADER;
}

/*
 * 解析请求头部
 */
void
http_parse_header(rb_channel_t *chl, size_t len)
{
    HTTP_REQUEST *req = (HTTP_REQUEST *)chl->user.data;
    char *p = rb_buffer_begin(chl->input);
    char *ep = p + len;

    if (strcmp(p, "\r\n") == 0) {
        /* 头部解析完毕 */
        req->status = HTTP_OK;
        return;
    } else
        req->status = HTTP_HEADER;

    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;

    int i = 0, j;
    if (strncasecmp(p, "Host:", 5) == 0) {
        p += 5;
        while (p < ep && (*p == ' ' || *p == '\t'))
            p++;
        j = sizeof(req->host) - 1;
        while (p < ep && i < j && !isspace(*p))
            req->host[i++] = *p++;
        req->host[i] = '\0';
    } else if (strncasecmp(p, "Connection:", 11) == 0) {
        p += 11;
        while (p < ep && (*p == ' ' || *p == '\t'))
            p++;
        j = sizeof(req->conn) - 1;
        while (p < ep && i < j && !isspace(*p))
            req->conn[i++] = *p++;
        req->conn[i] = '\0';
    } else {
        ; /* ignore */
    }
}

static void
fcat(rb_channel_t *chl, rb_buffer_t *buf, const char *name)
{
    int fd = open(name, O_RDONLY);
    if (fd > 0) {
        http_ok(chl, buf);
        while (rb_read_fd(buf, fd) > 0)
            http_send(chl, buf);
    } else
        http_not_found(chl, buf);
    close(fd);
}

void
http_response(rb_channel_t *chl)
{
    HTTP_REQUEST *req = (HTTP_REQUEST *)chl->user.data;
    rb_buffer_t *buf = rb_buffer_init();

    switch (req->status) {
    case HTTP_OK:
        if (strcasecmp(req->conn, "Keep-Alive") == 0)
            req->flag |= HTTP_ALIVE;
        if (strcasecmp(req->method, "GET") == 0) {
            fcat(chl, buf, req->url);
        } else
            http_unimplemented(chl, buf);
        req->status = HTTP_LINE;
        break;
    case HTTP_ERROR:
        http_not_found(chl, buf);
        req->status = HTTP_LINE;
        break;
    default:
        break;
    }
    rb_buffer_destroy(&buf);
}
