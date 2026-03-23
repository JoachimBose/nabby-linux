#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>
#include <assert.h>

#include "application.h"
#include "http_err.h"

#define PORT 8084
#define MAX_HTTP_HEADERS_SIZE 8192
#define MAX_PATH_LEN 128

//TODO: Regex misses ] in the path indicator.
#define HTTP_REGEX_PATTERN "^GET \\(/[-[:alpha:][:digit:]\._/?:#@!\$&'()\*+,;=%~\[]*\\) HTTP\/1\.1\r\n\\([^\r\n]\\{1,\\}\r\n\\)\\{1,\\}\r\n"

#define HTTP_BODYSPLIT "\r\n\r\n"

#define NMATCHES 3

regex_t http_regex;

int init_sock() {
  int server_fd;
  ssize_t valread;
  struct sockaddr_in address;
  int opt = 1;
  socklen_t addrlen = sizeof(address);

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 8080
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
    send_static(conn, HTTP_ERR);
    printf("invalid\n");
    return 0;
  }
  else if (r == REG_NOMATCH || (matches[1].rm_eo - matches[1].rm_so >= MAX_PATH_LEN - 1)) {
    send_static(conn, HTTP_BADREQ);
    printf("no match\n");
    printf("%s", hdr);
    return 0;
  }

  assert(matches[1].rm_so > 0 && matches[1].rm_eo < MAX_HTTP_HEADERS_SIZE);
  int path_size = matches[1].rm_eo - matches[1].rm_so >= MAX_PATH_LEN - 1;
  memset(path, 0, MAX_PATH_LEN);
  memcpy(path, hdr + matches[1].rm_so, path_size);

  return 1;
}

void handle_connection(int conn){
  char hdr[MAX_HTTP_HEADERS_SIZE] = {};
  char path[MAX_PATH_LEN] = {};
  for (int i = 0; i < MAX_HTTP_HEADERS_SIZE - 1; i++) { //read all the headers
    ssize_t res = recv(conn, hdr + i, 1, 0);
    if (res < 0) {
      perror("recv\n");
      close(conn);
      return;
    }
    if (res == 0) {
      printf("they hung up!\n");
      close(conn);
      return;
    }

    if (i >= 3 && strcmp(hdr + (i - 3), HTTP_BODYSPLIT) == 0) {
      break;
    }
  }
  extract_path(hdr, path, conn);
  printf("GET %s\n", path);
  respond(path, conn);
}

int main() {
  init_regexes();

  int connection = 0;
  int server_fd = init_sock();
  struct sockaddr_in address;
  socklen_t addrlen;


  while (1) {
    connection = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (connection < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    handle_connection(connection);
  }
  return 0;
}