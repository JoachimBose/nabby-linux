#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <assert.h>

#include "application.h"
#include "http_err.h"

#define PORT 8080
#define MAX_HTTP_HEADERS_SIZE 2048
#define MAX_PATH_LEN 128
#define MAX_CLIENTS 64
#define MAX_EVENTS  64

//TODO: Regex misses ] in the path indicator.
#define HTTP_REGEX_PATTERN "^GET \\(/[-[:alpha:][:digit:]\._/?:#@!\$&'\(\)\*+,;=%~\[]*\\) HTTP\/1\.1\r\n\\([^\r\n]\\{1,\\}\r\n\\)\\{1,\\}\r\n"

#define HTTP_BODYSPLIT "\r\n\r\n"

#define NMATCHES 3

regex_t http_regex;

typedef struct {
  int  fd;
  char hdr[MAX_HTTP_HEADERS_SIZE];
  int  len;
} client_state_t;

static client_state_t clients[MAX_CLIENTS];

static void clients_init(void) {
  for (int i = 0; i < MAX_CLIENTS; i++)
    clients[i].fd = -1;
}

static int client_alloc(int fd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].fd == -1) {
      clients[i].fd  = fd;
      clients[i].len = 0;
      memset(clients[i].hdr, 0, MAX_HTTP_HEADERS_SIZE);
      return i;
    }
  }
  return -1;
}

static int client_find(int fd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].fd == fd) return i;
  }
  return -1;
}

static void client_close(int idx, int epoll_fd) {
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clients[idx].fd, NULL);
  close(clients[idx].fd);
  clients[idx].fd = -1;
}

int init_sock() {
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;

  if ((server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family      = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  return server_fd;
}

void print_regerror(int rc, regex_t *preg){
  char buffer[100];
  regerror(rc, preg, buffer, sizeof(buffer));
  printf("%s\n", buffer);
}

void init_regexes(){
  int r = regcomp(&http_regex, HTTP_REGEX_PATTERN, 0);
  if(r != 0){
    print_regerror(r, &http_regex);
    perror("Regex failure\n");
    exit(EXIT_FAILURE);
  }
}

int extract_path(char* hdr, char* path, int conn){
  regmatch_t matches[NMATCHES];
  int r = regexec(&http_regex, hdr, NMATCHES, matches, 0);

  if (r != 0)
  {
    print_regerror(r, &http_regex);
    send_static(conn, HTTP_BADREQ);
    printf("invalid\n");
    return 0;
  }
  else if (r == REG_NOMATCH || (matches[1].rm_eo - matches[1].rm_so >= MAX_PATH_LEN - 1)) {
    send_static(conn, HTTP_ERR);
    printf("no match\n");
    printf("%s", hdr);
    return 0;
  }

  assert(matches[1].rm_so > 0 && matches[1].rm_eo < MAX_HTTP_HEADERS_SIZE);
  int path_size = matches[1].rm_eo - matches[1].rm_so;

  memset(path, 0, MAX_PATH_LEN);
  memcpy(path, hdr + matches[1].rm_so, path_size);

  return 1;
}

/* Read available bytes into the client's header buffer.
 * Returns 1 when \r\n\r\n is found, 0 if more data needed, -1 on error/close. */
static int client_read(int idx) {
  client_state_t *c = &clients[idx];

  while (c->len < MAX_HTTP_HEADERS_SIZE - 1) {
    ssize_t res = recv(c->fd, c->hdr + c->len, 1, MSG_DONTWAIT);
    if (res < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return 0;
      perror("recv");
      return -1;
    }
    if (res == 0) {
      printf("they hung up!\n");
      return -1;
    }
    c->len++;
    if (c->len >= 4 &&
        strcmp(c->hdr + (c->len - 4), HTTP_BODYSPLIT) == 0)
      return 1;
  }
  /* Buffer full without end of headers */
  send_static(c->fd, HTTP_BADREQ);
  return -1;
}

static void handle_ready_client(int idx, int epoll_fd) {
  int r = client_read(idx);
  if (r < 0) {
    client_close(idx, epoll_fd);
    return;
  }
  if (r == 0)
    return; /* headers not yet complete, keep waiting */

  /* Headers complete — dispatch and close */
  char path[MAX_PATH_LEN] = {};
  int conn = clients[idx].fd;
  if (extract_path(clients[idx].hdr, path, conn)) {
    printf("GET %s\n", path);
    respond(path, clients[idx].hdr, conn);
  }
  client_close(idx, epoll_fd);
}

int main() {
  init_regexes();
  clients_init();

  int server_fd = init_sock();

  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  struct epoll_event ev = { .events = EPOLLIN, .data.fd = server_fd };
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
    perror("epoll_ctl server");
    exit(EXIT_FAILURE);
  }

  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n < 0) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      if (fd == server_fd) {
        /* Accept all pending connections */
        while (1) {
          struct sockaddr_in addr;
          socklen_t addrlen = sizeof(addr);
          int conn = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
          if (conn < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept");
            break;
          }

          int idx = client_alloc(conn);
          if (idx < 0) {
            fprintf(stderr, "too many clients\n");
            close(conn);
            continue;
          }

          struct epoll_event cev = { .events = EPOLLIN, .data.fd = conn };
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn, &cev) < 0) {
            perror("epoll_ctl client");
            client_close(idx, epoll_fd);
          }
        }
      } else {
        int idx = client_find(fd);
        if (idx >= 0)
          handle_ready_client(idx, epoll_fd);
      }
    }
  }
  return 0;
}
