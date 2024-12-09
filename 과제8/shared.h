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
#include <sys/sem.h>
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
    int exit_flag; // 종료 플래그
    int valid_input; // 유효한 입력인지 확인하는 플래그
};


#define SHM_KEY_TRIGGER 0x1234
#define SHM_KEY_PRODUCER 0x5678
#define SEM_KEY_TRIGGER_PRODUCER 0x9ABC
#define SEM_KEY_PRODUCER_CONSUMER 0xDEF0

void sem_signal(int sem_id) {
    struct sembuf op = {0, 1, 0};
    semop(sem_id, &op, 1);
}

void sem_wait(int sem_id) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

#endif
