
#define MAX_ARG 6
typedef int (*handler_t)(int, int, char **argslist);

int attract_ghost_handler(int x, int y, char** argslist);
int do_handler(int x, int y, char** argslist);
int set_handler(int x, int y, char** argslist);