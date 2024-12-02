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
    int exit_flag;   // 종료 플래그
    int valid_input; // 유효한 입력 플래그
};

void sem_signal(int sem_id) {
    struct sembuf op = {0, 1, 0};
    semop(sem_id, &op, 1);
}

void sem_wait(int sem_id) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

#endif