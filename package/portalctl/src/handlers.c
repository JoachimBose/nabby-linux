#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "handlers.h"

#define MAX_OUT 128

static void write_ghost_device(const char *msg) {
    int fd = open("/dev/ghost_transmitter", O_WRONLY);
    if (fd >= 0) {
        write(fd, msg, strnlen(msg, MAX_OUT));
        close(fd);
    }
}

void printargs(char** argslist){
    for (int i = 0; i < MAX_ARG; i++) {
        printf(" * %s\n", argslist[i]);
    }
    printf("\n");
}

int set_handler(int x, int y, char** argslist){
    printf("sethandler\n");
    printargs(argslist);

    if (!argslist || !argslist[0] || !argslist[1]) {
        return -1;
    }

    char buf[MAX_OUT];
    snprintf(buf, sizeof(buf) - 1, "SET %s %s\n", argslist[0], argslist[1]);

    int fd = open("/etc/spectral_runtime.conf", O_WRONLY | O_CREAT, 0644);
    if (fd >= 0) {
        write(fd, buf, strnlen(buf, MAX_OUT));
        close(fd);
    }

    write_ghost_device("cfg_update\n");

    return 0;
}

int do_handler(int x, int y, char** argslist){
    printf("dohandler\n");
    printargs(argslist);

    if (!argslist || !argslist[0]) {
        return -1;
    }

    char buf[MAX_OUT];
    snprintf(buf, sizeof(buf) - 1, "DO %s\n", argslist[0]);

    int fd = open("/etc/spectral_actions.log", O_WRONLY | O_CREAT, 0644);
    if (fd >= 0) {
        write(fd, buf, strnlen(buf, MAX_OUT));
        close(fd);
    }

    write_ghost_device("ritual_exec\n");

    return 0;
}

int attract_ghost_handler(int x, int y, char** argslist){
    printf("attract ghosts\n");
    printargs(argslist);

    const char *target = "unknown";
    if (argslist && argslist[0]) {
        target = argslist[0];
    }

    char buf[MAX_OUT];
    snprintf(buf, sizeof(buf) - 1, "ATTRACT %s\n", target);

    int fd = open("/etc/spectral_events.log", O_WRONLY | O_CREAT, 0644);
    if (fd >= 0) {
        write(fd, buf, strnlen(buf, MAX_OUT));
        close(fd);
    }

    write_ghost_device("come here!");

    return 0;
}