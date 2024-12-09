#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 3
#define BUF_SIZE 1024

// 구조체 정의
typedef struct {
    int num1;
    int num2;
    char operator;
    char str[BUF_SIZE];
} ClientData;

typedef struct {
    int min;
    int max;
    struct tm timestamp;
    struct sockaddr_in client_addr;
} ServerData;

// 클라이언트와 서버 간에 공유할 힙 메모리
ClientData *client_data[MAX_CLIENTS];
ServerData *server_data[MAX_CLIENTS];

// 수식 계산 함수
int calculate(int num1, int num2, char operator) {
    int result = 0;
    switch (operator) {
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
        case '/': 
            if (num2 != 0) result = num1 / num2;
            else {
                printf("Zero division error\n");
                return -1;
            }
            break;
        default:
            printf("Unknown operator\n");
            return -1;
    }
    return result;
}

// 현재 시간 구하기
void get_current_time(struct tm *t) {
    time_t now;
    time(&now);
    localtime_r(&now, t);
}

// 워커 스레드 함수
void *worker_thread(void *arg) {
    int client_id = *(int *)arg;
    ClientData *client = client_data[client_id];
    ServerData *server = server_data[client_id];

    while (1) {
        // 수식 계산
        int result = calculate(client->num1, client->num2, client->operator);
        if (result == -1) return NULL;

        // 현재 시간 구하기
        struct tm t;
        get_current_time(&t);

        // min, max 갱신
        if (result < server->min) server->min = result;
        if (result > server->max) server->max = result;

        // 서버 데이터 출력
        printf("%d%c%d=%d %s min=%d max=%d %s from %s\n",
            client->num1, client->operator, client->num2, result, 
            client->str, server->min, server->max, 
            asctime(&server->timestamp), inet_ntoa(server->client_addr.sin_addr));

        // 클라이언트로 데이터 전송
        send(client->num1, &server->min, sizeof(server->min), 0);

        sleep(10); // 10초 대기
    }

    return NULL;
}

// 서버 코드
int main() {
    int server_sock, client_sock[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pthread_t threads[MAX_CLIENTS];
    int client_id[MAX_CLIENTS];

    // 서버 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Server socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 바인딩
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(1);
    }

    // 리슨 시작
    if (listen(server_sock, MAX_CLIENTS) == 0) {
        printf("Server listening on port 12345...\n");
    } else {
        perror("Listen failed");
        exit(1);
    }

    // 클라이언트 연결 수락
    for (int i = 0; i < MAX_CLIENTS; i++) {
        addr_size = sizeof(client_addr);
        client_sock[i] = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock[i] < 0) {
            perror("Client connection failed");
            exit(1);
        }

        printf("Client %d connected\n", i + 1);

        // 클라이언트 데이터 및 서버 데이터 초기화
        client_data[i] = malloc(sizeof(ClientData));
        server_data[i] = malloc(sizeof(ServerData));
        server_data[i]->min = 99999;
        server_data[i]->max = -99999;
        server_data[i]->client_addr = client_addr;
        server_data[i]->timestamp = *localtime(&(time_t){time(NULL)});

        // 클라이언트로부터 데이터 받기
        recv(client_sock[i], client_data[i], sizeof(ClientData), 0);

        // 워커 스레드 생성
        client_id[i] = i;
        pthread_create(&threads[i], NULL, worker_thread, &client_id[i]);
    }

    // 메인 스레드는 클라이언트 연결을 계속 대기
    while (1) {
        sleep(1); // 서버 대기
    }

    // 종료시 자원 해제
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
        free(client_data[i]);
        free(server_data[i]);
    }

    close(server_sock);
    return 0;
}
