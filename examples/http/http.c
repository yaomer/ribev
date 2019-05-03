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
    char *s = "HTTP/1.1 200 OK\r\n";
    rb_buffer_write(buf, s, strlen(s));
    s = "Ribev-Server\r\n";
    rb_buffer_write(buf, s, strlen(s));
    s = "Content-Type: text/html\r\n";
    rb_buffer_write(buf, s, strlen(s));
    s = "\r\n";
    rb_buffer_write(buf, s, strlen(s));
}

static void
http_not_found(rb_buffer_t *buf)
{
    char *s = "HTTP/1.1 404 NOT FOUND\r\n";
    rb_buffer_write(buf, s, strlen(s));
    s = "Ribev-Server\r\n";
    rb_buffer_write(buf, s, strlen(s));
    s = "Content-Type: text/html\r\n";
    rb_buffer_write(buf, s, strlen(s));
    s = "\r\n";
    rb_buffer_write(buf, s, strlen(s));
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
    while (p < ep && !isspace(*p))
        req->method[i++] = *p++;
    req->method[i] = '\0';
    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;
    /* url */
    i = 0;
    req->url[i++] = '.';
    while (p < ep && !isspace(*p))
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
    while (p < ep && !isspace(*p))
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
    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;

    int i = 0;
    while (p < ep) {
        if (strcasecmp(p, "Host:") == 0) {
            p += 5;
            while (p < ep && !isspace(*p))
                req->host[i++] = *p++;
            req->host[i] = '\0';
        } else {
            /* ignore */
            break;
        }
    }
    req->status = HTTP_OK;
}

static void
fcat(rb_buffer_t *buf, const char *name)
{
    int fd = open(name, O_RDONLY);
    if (fd > 0) {
        http_header(buf);
        rb_read_fd(buf, fd);
    } else
        http_not_found(buf);
    close(fd);
}

void
http_response(rb_channel_t *chl)
{
    HTTP_REQUEST *req = (HTTP_REQUEST *)chl->user.data;
    rb_buffer_t *buf = rb_buffer_init();

    switch (req->status) {
    case HTTP_OK:
        if (strcasecmp(req->method, "GET") == 0) {
            fcat(buf, req->url);
        } else
            ; /* 不支持 */
        break;
    case HTTP_ERROR:
        http_not_found(buf);
        break;
    default:
        break;
    }

    if (req->status) {
        char *s = rb_buffer_begin(buf);
        size_t len = rb_buffer_readable(buf);
        rb_send(chl, s, len);
        req->status = 0;
    }
    rb_buffer_destroy(&buf);
}