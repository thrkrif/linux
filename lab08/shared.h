#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>

#define NAME_LEN 20

struct calculation_data {
    int left_num;
    int right_num;
    char op;
    char name[NAME_LEN];
    int result;
    int min;
    int max;
    struct tm timestamp;
    struct sockaddr_in producer_addr;
};

#define SHM_KEY_TRIGGER 0x1234
#define SHM_KEY_PRODUCER 0x5678

#endif
