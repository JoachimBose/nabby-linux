
#include <assert.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "http_err.h"
#include "websitelib.h"
#define PROC_RESPONSE_SIZE 2048

#define AUTH_USER     "admin"
#define AUTH_PASS_LEN 14

static const uint8_t AUTH_PASS_KEY[AUTH_PASS_LEN] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE,
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE,
    0xDE, 0xAD
};

static const uint8_t AUTH_PASS_ENC[AUTH_PASS_LEN] = {
    0xB0, 0x99, 0xDC, 0x8D, 0xB3, 0xA1,
    0xAD, 0xDD, 0x8D, 0x8C, 0xBE, 0x8C,
    0xBF, 0xC1
};

static char current_session_token[33] = {0};

static void generate_session_token(void) {
    uint32_t t = (uint32_t)time(NULL);
    uint32_t a = t ^ 0xDEADBEEF;
    uint32_t b = (t >> 8) ^ 0xCAFEBABE;
    uint32_t c = a ^ (b << 5);
    snprintf(current_session_token, sizeof(current_session_token),
             "%08x%08x%08x%08x", t, a, b, c);
}

static const char *login_page =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "<title>BeyondLink - Authentication Required</title>\n"
    "<style>\n"
    "  *{box-sizing:border-box;}\n"
    "  body{margin:0;font-family:'Courier New',monospace;\n"
    "    background:radial-gradient(circle at center,#0a0a0f,#000000);\n"
    "    color:#9effff;display:flex;flex-direction:column;\n"
    "    align-items:center;justify-content:center;min-height:100vh;}\n"
    "  header{padding:20px;text-align:center;border-bottom:1px solid #0ff;\n"
    "    text-shadow:0 0 10px #0ff;width:100%;margin-bottom:40px;}\n"
    "  header h1{margin:0 0 8px;}\n"
    "  header p{margin:4px 0;}\n"
    "  .auth-card{border:1px solid #0ff;padding:40px;\n"
    "    background:rgba(0,255,255,0.05);box-shadow:0 0 20px #0ff33a;\n"
    "    min-width:340px;}\n"
    "  .auth-card h2{margin:0 0 24px;text-align:center;\n"
    "    text-shadow:0 0 8px #0ff;font-size:1.1em;letter-spacing:0.1em;}\n"
    "  .field{margin-bottom:18px;}\n"
    "  label{display:block;margin-bottom:6px;font-size:0.8em;\n"
    "    letter-spacing:0.15em;}\n"
    "  input{width:100%;padding:10px;background:rgba(0,0,0,0.8);\n"
    "    border:1px solid #0ff3;color:#9effff;\n"
    "    font-family:'Courier New',monospace;font-size:1em;outline:none;\n"
    "    transition:border-color 0.2s,box-shadow 0.2s;}\n"
    "  input:focus{border-color:#0ff;box-shadow:0 0 8px #0ff5;}\n"
    "  button{width:100%;padding:12px;background:rgba(0,255,255,0.08);\n"
    "    border:1px solid #0ff;color:#9effff;\n"
    "    font-family:'Courier New',monospace;font-size:1em;\n"
    "    cursor:pointer;letter-spacing:0.2em;transition:all 0.2s;}\n"
    "  button:hover{background:rgba(0,255,255,0.18);box-shadow:0 0 12px #0ff;}\n"
    "  .errmsg{color:#ff4d4d;text-shadow:0 0 5px red;text-align:center;\n"
    "    margin-bottom:16px;font-size:0.9em;display:none;}\n"
    "  footer{margin-top:40px;font-size:0.75em;color:#444;text-align:center;}\n"
    "</style>\n"
    "</head>\n"
    "<body>\n"
    "<header>\n"
    "  <h1>BeyondLink Spectral Interface</h1>\n"
    "  <p>Ghost Communication &amp; Monitoring System</p>\n"
    "  <p><i>Restricted Access -- Authorized Personnel Only</i></p>\n"
    "</header>\n"
    "<div class=\"auth-card\">\n"
    "  <h2>[ OPERATOR LOGIN ]</h2>\n"
    "  <p id=\"err\" class=\"errmsg\">ACCESS DENIED: Invalid credentials</p>\n"
    "  <form action=\"/auth\" method=\"get\">\n"
    "    <div class=\"field\">\n"
    "      <label>USERNAME</label>\n"
    "      <input type=\"text\" name=\"user\" autocomplete=\"off\" spellcheck=\"false\">\n"
    "    </div>\n"
    "    <div class=\"field\">\n"
    "      <label>PASSWORD</label>\n"
    "      <input type=\"password\" name=\"pass\" autocomplete=\"off\">\n"
    "    </div>\n"
    "    <button type=\"submit\">AUTHENTICATE</button>\n"
    "  </form>\n"
    "</div>\n"
    "<footer>&copy; 1987 BeyondLink Systems</footer>\n"
    "<script>\n"
    "  if(new URLSearchParams(location.search).get('failed')==='1'){\n"
    "    document.getElementById('err').style.display='block';\n"
    "  }\n"
    "</script>\n"
    "</body>\n"
    "</html>\n";

void respond_html(int conn, const char *html) {
  int content_len = strlen(html);
  dprintf(conn, HTTP_OK_TEMPLT, content_len);
  write(conn, html, content_len);
}

/* Compare path base (ignoring query string) against target */
static int path_base_eq(const char *path, const char *target) {
  size_t tlen = strlen(target);
  return (strncmp(path, target, tlen) == 0) &&
         (path[tlen] == '\0' || path[tlen] == '?');
}

/* Extract a single query parameter value from a path like /foo?a=1&b=2 */
static int get_query_param(const char *path, const char *param,
                           char *out, size_t out_size) {
  const char *q = strchr(path, '?');
  if (!q) return 0;
  q++;

  char needle[64];
  snprintf(needle, sizeof(needle) - 1, "%s=", param);
  const char *p = strstr(q, needle);
  if (!p) return 0;
  p += strlen(needle);

  size_t i = 0;
  while (*p && *p != '&' && i < out_size - 1) {
    out[i++] = *p++;
  }
  out[i] = '\0';
  return 1;
}

/* Check if the Cookie header in `headers` carries our session token */
static int check_session(const char *headers) {
  if (current_session_token[0] == '\0') return 0;

  const char *c = strstr(headers, "Cookie:");
  if (!c) return 0;
  c = strstr(c, "session=");
  if (!c) return 0;
  c += 8; /* strlen("session=") */

  size_t tlen = strlen(current_session_token);
  return (strncmp(c, current_session_token, tlen) == 0) &&
         (c[tlen] == ';' || c[tlen] == '\r' || c[tlen] == '\n' ||
          c[tlen] == ' ' || c[tlen] == '\0');
}

/* Verify username and XOR-decoded password */
static int check_credentials(const char *user, const char *pass) {
  if (strcmp(user, AUTH_USER) != 0) return 0;
  if (strlen(pass) != AUTH_PASS_LEN) return 0;
  for (int i = 0; i < AUTH_PASS_LEN; i++) {
    if (((uint8_t)pass[i] ^ AUTH_PASS_KEY[i]) != AUTH_PASS_ENC[i]) return 0;
  }
  return 1;
}

int is_digits(const char *str) {
  for (int i = 0; i < strlen(str); i++) {
    if (str[i] < 0x30 || str[i] > 0x39) {
      return 0;
    }
  }
  return 1;
}

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

void respond_proc_pid(char *pidstr, int conn) {
  char outputbuff[2048] = {};

  snprintf(outputbuff, sizeof(outputbuff), "cat /proc/%s/status", pidstr);
  FILE *f = popen(outputbuff, "r");
  if (f == NULL) {
    send_static(conn, HTTP_ERR);
    return;
  }
  memset(outputbuff, 0, sizeof(outputbuff));

  if (fread(outputbuff, 1, sizeof(outputbuff), f) == 0) {
    perror("fread on popen\n");
    send_static(conn, HTTP_ERR);
    return;
  }

  dprintf(conn, HTTP_OK_TEMPLT, (int)strlen(outputbuff));
  write(conn, outputbuff, strlen(outputbuff));
  return;
}

//? Auth responder
static void respond_auth(const char *path, int conn) {
  char user[64] = {0};
  char pass[64] = {0};

  get_query_param(path, "user", user, sizeof(user));
  get_query_param(path, "pass", pass, sizeof(pass));

  if (check_credentials(user, pass)) {
    generate_session_token();
    dprintf(conn, HTTP_REDIRECT_WITH_COOKIE, current_session_token);
  } else {
    send_static(conn, HTTP_REDIRECT_LOGIN_FAIL);
  }
  close(conn);
}


//? Main stuff
#define PROCS_ENDPOINT           "/procs"
#define SYSTEM_TELEMETRY_ENDPOINT "/telemetry"
#define PROC_PID_ENDPOINT        "/proc/"
#define AUTH_ENDPOINT            "/auth"

void respond(char *path, const char *headers, int conn) {
  /* Login endpoint — no session required */
  if (path_base_eq(path, AUTH_ENDPOINT)) {
    respond_auth(path, conn);
    return;
  }

  /* Root: show login page (or homepage if authenticated) */
  if (path_base_eq(path, "/")) {
    if (!check_session(headers)) {
      respond_html(conn, login_page);
    } else {
      respond_html(conn, gethomepage());
    }
    return;
  }

  /* All remaining endpoints require a valid session */
  if (!check_session(headers)) {
    send_static(conn, HTTP_REDIRECT_LOGIN);
    return;
  }

  if (path_base_eq(path, PROCS_ENDPOINT)) {
    respond_procs(conn);
    return;
  } else if (path_base_eq(path, SYSTEM_TELEMETRY_ENDPOINT)) {
    respond_html(conn, getsystemtelemetry());
    return;
  } else if (strncmp(path, PROC_PID_ENDPOINT, strlen(PROC_PID_ENDPOINT)) == 0) {
    respond_proc_pid(path + strlen(PROC_PID_ENDPOINT), conn);
    return;
  } else {
    send_static(conn, HTTP_NOTFOUND);
    return;
  }
  assert(0);
}
