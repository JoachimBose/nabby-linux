
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "http_err.h"
#include "websitelib.h"
#define PROC_RESPONSE_SIZE 2048

void respond_html(int conn, const char *html) {
  int content_len = strlen(html);
  dprintf(conn, HTTP_OK_TEMPLT, content_len);
  write(conn, html, content_len);
}

int is_digits(const char *str) {
  for (int i = 0; i < strlen(str); i++) {
    if (str[i] < 0x30 || str[i] > 0x39) {
      return 0;
    }
  }
  return 1;
}

/*
PLAN:
 for every valid procfs entry:
  add a line to the buffer containing:
    the pid
    the cmdline of the proc

  for buffer_filled is the ptr into the buffer, so
*/

#define TMPBUF_SIZE 64

size_t add_proc_entry(const char *pid_str, char *response_buffer,
                      size_t buffer_filled, size_t buffer_size) {
  size_t i = buffer_filled;
  char tmpbuf[TMPBUF_SIZE] = {};

  snprintf(tmpbuf, TMPBUF_SIZE - 1, "/proc/%s/comm", pid_str);
  FILE *f = fopen(tmpbuf, "r");
  if (f == NULL) {
    return 0;
  }

  memset(tmpbuf, 0, TMPBUF_SIZE);
  ssize_t res = fread(tmpbuf, TMPBUF_SIZE - 3, 1, f);
  if (res < 0) {
    fclose(f);
    perror("fread\n");
    return 0;
  }
  tmpbuf[strlen(tmpbuf) - 1] = '\0';

  if (strlen(pid_str) + strlen(tmpbuf) + 27 + i < buffer_size) {
    i += snprintf(response_buffer + i, buffer_size - buffer_filled - 1,
                  "{\"pid\":\"%s\",\n\"comm\":\"%s\"},\n", pid_str, tmpbuf);
  } else {
    i = 0;
  }

  fclose(f);
  return i;
}

#define PROCS_PREAMBLE "{\"procs\":[\n"

void respond_procs(int conn) {
  DIR *dp;
  struct dirent *ep;
  char response_buffer[PROC_RESPONSE_SIZE] = {};
  size_t i = 0;
  size_t rslt = 0;
  ssize_t res = 0;
  int r = 0;

  i += snprintf(response_buffer, sizeof(response_buffer) - 1, PROCS_PREAMBLE);

  dp = opendir("/proc/");
  if (dp == NULL) {
    send_static(conn, HTTP_ERR);
    goto out;
  }

  while ((ep = readdir(dp)) != NULL) {
    if (ep->d_type != DT_DIR || is_digits(ep->d_name) == 0) {
      continue;
    }
    rslt = add_proc_entry(ep->d_name, response_buffer, i, PROC_RESPONSE_SIZE);
    if (rslt == 0) {
      break;
    } else {
      i = rslt;
    }
  }

  assert(i + 3 < sizeof(response_buffer));
  snprintf(response_buffer + i - 2, sizeof(response_buffer) - 1, "]}\n");
  r = dprintf(conn, HTTP_JSON_RESPONSE_TEMPLT,
              (int)strnlen(response_buffer, sizeof(response_buffer) - 1));
  if (r < 0) {
    perror("write\n");
    goto out;
  }

  res = write(conn, response_buffer, sizeof(response_buffer) - 1);
  if (res < 0) {
    perror("write\n");
    goto out;
  }

out:
  closedir(dp);
  return;
}

void respond_proc_pid(char* pidstr, int conn){
  /*
  PLAN:
   * construct command
   * Use popen
   * use a read/write loop of singular bytes
  */
  char outputbuff[2048] = {};
  
  snprintf(outputbuff, sizeof(outputbuff), "cat /proc/%s/status", pidstr);
  FILE* f = popen(outputbuff, "r");
  if (f == NULL) {
    send_static(conn, HTTP_ERR);
    return;
  }
  memset(outputbuff, 0, sizeof(outputbuff));

  char c = 0;
  if(fread(outputbuff, 1, sizeof(outputbuff), f) == 0){
    perror("fread on popen\n");
    send_static(conn, HTTP_ERR);
    return;
  }

  dprintf(conn, HTTP_OK_TEMPLT, (int)strlen(outputbuff));
  write(conn, outputbuff, strlen(outputbuff));
  return;
}

#define PROCS_ENDPOINT "/procs"
#define SYSTEM_TELEMETRY_ENDPOINT "/telemetry"
#define PROC_PID_ENDPOINT "/proc/"

void respond(char *path, int conn) {
  if (strcmp(path, "/") == 0) {
    respond_html(conn, gethomepage());
    return;
  }
  else if (strcmp(path, PROCS_ENDPOINT) == 0) {
    respond_procs(conn);
    return;
  } else if (strcmp(path, SYSTEM_TELEMETRY_ENDPOINT) == 0) {
    respond_html(conn, getsystemtelemetry());
    return;
  } else if (strncmp(path, PROC_PID_ENDPOINT, strlen(PROC_PID_ENDPOINT)) == 0) {
    respond_proc_pid(path + strlen(PROC_PID_ENDPOINT), conn);
    return;
  }
  else {
    send_static(conn, HTTP_NOTFOUND);
    return;
  }
  assert(0);
}