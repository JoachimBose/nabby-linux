#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "handlers.h"

/*
PLAN:
 * arbitrary call -> some kind of vtables or something, but no variables are
available.

 * We make some kind of DSL for configuring the spectral interface. A
ghostfilter if you will. It will parse the DSL into some kind of AST. Then it
will overflow or typeconfuse an AST node or something. Then we can make a rogue
node and an arbitrary call to something. This arbitrary call will have to
somehow turn into a stack pivot using: something like this: `sub sp, fp, #4; pop
{fp, pc}; `
*/

#define INFILE_NAME "/etc/graveyard/config"
#define MAX_LINE_LEN 2048

#define CMD_SET "SET"
#define CMD_DO "DO"
#define MAP_ADDR (void *)0x2100000

__attribute__((unused)) void gadgets() {
  asm volatile("sub sp, fp, #0xc\n\t"
               "pop {r4, r5, fp, pc}" ::
                   :);
  asm volatile("add sp, sp, #0x8\n\t"
               "pop {r4, r5, fp, pc}" ::
                   :);
  asm volatile("svc 0\n\t":::);
}
//region helpers

int __attribute__((noinline,noclone)) _is_space(char c){
  return c == ' ';
}

char *prep_file(char **endptr) {
  int fd = open(INFILE_NAME, O_CLOEXEC, O_RDONLY);
  if (fd < 0) {
    perror("failed to open " INFILE_NAME "\n");
    exit(EXIT_FAILURE);
  }

  struct stat sb = {};
  int r = fstat(fd, &sb);
  if (r == -1) {
    perror("failed to stat\n");
    exit(EXIT_FAILURE);
  }
  size_t file_maxsize = sb.st_size;

  char *f =
      mmap(MAP_ADDR, file_maxsize, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0x0);
  if (f == NULL) {
    perror("mmap failed\n");
    exit(EXIT_FAILURE);
  }
  *endptr = f + file_maxsize;
  return f;
}

__attribute__((noinline,noclone)) char *skip_whitespace(char *start, char *max) {
  int i = 0;
  while (_is_space(start[i]) && start + i < max) {
    i++;
  }
  return start + i;
}

__attribute__((noinline,noclone)) char *skip_nonspace(char *start, char *max) {
  int i = 0;
  while (!_is_space(start[i]) && start + i < max) {
    i++;
  }
  return start + i;
}

#define _parse_cmd(X, Y)                                                       \
  if (strncmp(X, *parse_ptr, strlen(X)) == 0) {                                \
    (*parse_ptr) += strlen(X);                                                 \
    return Y;                                                                  \
  } else

handler_t __attribute__((noinline, noclone)) parse_cmd(char **parse_ptr,
                                                       char *end_ptr) {
  _parse_cmd(CMD_SET, set_handler);
  _parse_cmd(CMD_DO, do_handler) { 
    char* og = *parse_ptr;
    *parse_ptr = skip_nonspace(*parse_ptr, end_ptr);
    printf("uknown opcode %s\n", og);
  }
}

#undef _parse_cmd
//endregion
#define OPCODE_MAXLEN 20

struct stack_args {
  char arg_buf[OPCODE_MAXLEN];
  handler_t handler;
};

int main() {
  char *max;
  char *input = prep_file(&max);
  char *parse_ptr = input;
  char *arglist[MAX_ARG + 1] = {};
  struct stack_args sa;

  while (parse_ptr < max) {
    parse_ptr = skip_whitespace(parse_ptr, max);
    sa.handler = parse_cmd(&parse_ptr, max);
    if (sa.handler == 0x0) {
      printf("invalid config\n");
      return 0;
    }

    int c = 0;
    memset(arglist, 0, sizeof(arglist));

    while (parse_ptr < max && c < MAX_ARG) {
      if (*parse_ptr == ';') {
        parse_ptr++;
        break;
      }

      parse_ptr = skip_whitespace(parse_ptr, max);

      // get the next argparse_ptr
      size_t argsize = 0;
      for (; !_is_space(parse_ptr[argsize]) && argsize < OPCODE_MAXLEN; argsize++) {
        sa.arg_buf[argsize] = parse_ptr[argsize];
      }

      arglist[c] = calloc(argsize + 1, 1);
      memcpy(arglist[c], sa.arg_buf, argsize);
      parse_ptr += argsize;
      c++;
      parse_ptr = skip_whitespace(parse_ptr, max);
    }

    sa.handler(0, 0, arglist);
  }
}
