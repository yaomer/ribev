/*
 * 一个很简陋的http-server，仅支持GET请求
 */

#include "../usr_serv.h"
#include "http.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

static void
msgcb(rb_channel_t *chl)
{
    HTTP_REQUEST *http = (HTTP_REQUEST *)chl->user.data;

    /* printf("client request:\n%s", rb_buffer_begin(chl->input)); */
    while (rb_buffer_readable(chl->input) >= 2) {
        /* 每次返回一行，即[*\r\n] */
        int crlf = rb_find_crlf(chl->input);
        if (crlf >= 0) {
            switch (http->status) {
            case HTTP_LINE:
                http_parse_line(chl, crlf);
                crlf += 2;
                break;
            case HTTP_HEADER:
                http_parse_header(chl, crlf);
                crlf += 2;
                break;
            default:
                /* 只解析请求行和请求头部 */
                crlf = rb_buffer_readable(chl->input);
                break;
            }
            rb_buffer_retrieve(chl->input, crlf);
        } else
            break;
    }
    http_response(chl);
    if (!(http->flag & HTTP_ALIVE))
        chl->closecb(chl);
}

static void *
init(void)
{
    HTTP_REQUEST *http = rb_malloc(sizeof(HTTP_REQUEST));
    memset(http, 0, sizeof(*http));
    http->status = HTTP_LINE;
    return (void *)http;
}

static void
dealloc(void *data)
{
    rb_free(data);
}

static void
sig_int(int signo)
{
    printf("Good Bye!\n");
    exit(1);
}

int
main(void)
{
    rb_user_t user;
    user.init = init;
    user.dealloc = dealloc;
    signal(SIGINT, sig_int);
    rb_serv_t *serv = rb_serv_init(1);
    rb_serv_listen(serv, 8080, msgcb, NULL, user);
    printf("server is running at http://localhost:8080\n");
    rb_serv_run(serv);
}
