#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include "shared.h"

#define PORT 8080
#define MAX_CLIENTS 10

pthread_mutex_t lock;
pthread_cond_t cond;

struct client_data shared_data;
int is_data_ready = 0;

void *producer(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    struct client_data data;
    recv(client_socket, &data, sizeof(struct client_data), 0);

    // 연산 수행
    switch (data.op) {
        case '+':
            data.result = data.left_num + data.right_num;
            break;
        case '-':
            data.result = data.left_num - data.right_num;
            break;
        case 'x':
            data.result = data.left_num * data.right_num;
            break;
        case '/':
            data.result = (data.right_num != 0) ? data.left_num / data.right_num : 0;
            break;
        default:
            data.result = 0;
    }

    // 최소, 최대값 업데이트
    pthread_mutex_lock(&lock);
    if (is_data_ready == 0) { // 초기화
        shared_data.min = data.result;
        shared_data.max = data.result;
    }
    if (data.result < shared_data.min) shared_data.min = data.result;
    if (data.result > shared_data.max) shared_data.max = data.result;

    shared_data = data;
    is_data_ready = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);

    return NULL;
}

void *consumer(void *arg) {
    int client_socket = *(int *)arg;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // 서버의 IP 주소 가져오기
    getsockname(client_socket, (struct sockaddr *)&server_addr, &addr_len);
    char server_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, server_ip, INET_ADDRSTRLEN);

    while (1) {
        pthread_mutex_lock(&lock);

        // 데이터 준비 대기
        while (!is_data_ready) {
            pthread_cond_wait(&cond, &lock);
        }

        // 데이터 준비 완료 후 상태 초기화
        struct client_data *data_to_send = &shared_data;
        is_data_ready = 0;
        pthread_mutex_unlock(&lock);

        // 시간 정보 추가
        time_t now = time(NULL);
        struct tm *current_time = localtime(&now);

        // 서버 주소, 클라이언트 데이터, 시간 정보 전송
        send(client_socket, &server_addr, sizeof(struct sockaddr_in), 0);
        send(client_socket, data_to_send, sizeof(struct client_data), 0);
        send(client_socket, current_time, sizeof(struct tm), 0);

        // 10초 대기
        sleep(10);
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_fd, *new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 옵션 설정
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 바인드
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 대기
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d...\n", PORT);

    // 클라이언트 연결 처리
    while (1) {
        new_socket = malloc(sizeof(int));
        *new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (*new_socket < 0) {
            perror("Accept failed");
            free(new_socket);
            continue;
        }

        pthread_t producer_thread, consumer_thread;
        pthread_create(&producer_thread, NULL, producer, new_socket);
        pthread_create(&consumer_thread, NULL, consumer, new_socket);
        pthread_detach(producer_thread);
        pthread_detach(consumer_thread);
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    return 0;
}
