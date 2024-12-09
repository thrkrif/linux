#ifndef SHARED_H
#define SHARED_H

#include <time.h>
#include <netinet/in.h>

#define NAME_LEN 50

struct client_data {
    int left_num;
    int right_num;
    char op;
    char name[NAME_LEN];
    int result;
};

struct timestamp_data {
    struct tm current_time;
};

struct server_info {
    struct sockaddr_in server_addr;
    int min;
    int max;
};

#endif