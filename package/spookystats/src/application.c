#include "http_err.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "websitelib.h"

void respond_html(int conn, const char* html){
    int content_len = strlen(html);
    dprintf(conn, HTTP_OK_TEMPLT, content_len);
    write(conn, html, content_len);
}

void respond(char* path, int conn){
    if (strcmp(path, "/")) {
        respond_html(conn, gethomepage());
        return;
    }
    else {
        send_static(conn, HTTP_NOTFOUND);
    }
}