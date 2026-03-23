#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define HTTP_ERR "HTTP/1.1 500 Internal Server Error\r\n\
Content-Type: text/plain\r\n\
Content-Length: 25\r\n\
Connection: close\r\n\
\r\n\
500 Internal Server Error"

#define HTTP_BADREQ "HTTP/1.1 400 Bad Request\r\n\
Content-Type: text/plain\r\n\
Content-Length: 15\r\n\
Connection: close\r\n\
\r\n\
400 Bad Request"

#define HTTP_NOTFOUND "HTTP/1.1 404 Not Found\r\n\
Content-Type: text/plain\r\n\
Content-Length: 13\r\n\
Connection: close\r\n\
\r\n\
404 Not Found"

#define HTTP_OK_TEMPLT "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %d\r\n\r\n"


static inline void send_static(int conn, const char* problem_response){
  write(conn, problem_response, strlen(problem_response));
  close(conn);
  printf("sending problem..\n");
}