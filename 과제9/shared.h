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
    int exit_flag;   // 종료 플래그
    int valid_input; // 유효한 입력 플래그
};

// 타임스탬프 관련 데이터 구조체
struct timestamp_data {
    struct tm timestamp; // 계산 수행 시간
};

// 서버 정보 관련 데이터 구조체
struct server_info_data {
    char ip_address[INET_ADDRSTRLEN]; // 서버의 IP 주소
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

// #ifndef SHARED_H
// #define SHARED_H

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <time.h>
// #include <arpa/inet.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>
// #include <sys/sem.h>

// #define NAME_LEN 20

// // 계산 관련 데이터 구조체
// struct calculation_data {
//     int left_num;      // 왼쪽 피연산자
//     int right_num;     // 오른쪽 피연산자
//     char op;           // 연산자
//     char name[NAME_LEN]; // 사용자의 이름
//     int result;        // 계산 결과
//     int min;           // 최소값
//     int max;           // 최대값
//     int exit_flag;     // 종료 플래그
//     int valid_input;   // 유효한 입력 여부
// };

// // 타임스탬프 관련 데이터 구조체
// struct timestamp_data {
//     struct tm timestamp; // 계산 수행 시간
// };

// // 서버 정보 관련 데이터 구조체
// struct server_info_data {
//     char ip_address[INET_ADDRSTRLEN]; // 서버의 IP 주소
// };

// // 세마포어 신호 증가
// void sem_signal(int sem_id) {
//     struct sembuf op = {0, 1, 0};
//     semop(sem_id, &op, 1);
// }

// // 세마포어 신호 감소
// void sem_wait(int sem_id) {
//     struct sembuf op = {0, -1, 0};
//     semop(sem_id, &op, 1);
// }

// #endif

