#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define PORT 12345

// 구조체 정의
typedef struct {
    int num1;
    int num2;
    char operator;
    char str[BUF_SIZE];  // 문자열
    int min;
    int max;
} ClientData;

typedef struct {
    struct tm timestamp;  // 계산 시점
} CalculationTime;

typedef struct {
    struct sockaddr_in client_addr; // 클라이언트의 주소
} ServerInfo;

// 클라이언트와 통신하는 워커 스레드 함수
void* handle_client(void* arg) {
    int client_sock = *((int*)arg);
    ClientData client_data;
    CalculationTime calc_time;
    ServerInfo server_info;
    time_t rawtime;

    // 클라이언트와의 통신
    while (1) {
        // 클라이언트로부터 데이터 수신
        recv(client_sock, &client_data, sizeof(ClientData), 0);
        
        if (strcmp(client_data.str, "quit") == 0) {
            break;
        }

        // 계산 결과 및 최소/최대값 계산
        if (client_data.num1 > client_data.num2) {
            client_data.max = client_data.num1;
            client_data.min = client_data.num2;
        } else {
            client_data.max = client_data.num2;
            client_data.min = client_data.num1;
        }

        // 현재 시간 계산
        time(&rawtime);
        calc_time.timestamp = *localtime(&rawtime);

        // 서버 정보 (서버 IP주소) 설정
        server_info.client_addr.sin_family = AF_INET;
        server_info.client_addr.sin_port = htons(PORT);
        server_info.client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        // 클라이언트에게 구조체 전송
        send(client_sock, &client_data, sizeof(ClientData), 0);
        send(client_sock, &calc_time, sizeof(CalculationTime), 0);
        send(client_sock, &server_info, sizeof(ServerInfo), 0);

        // 10초 간격으로 계속 전송
        sleep(10);
    }

    close(client_sock);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 서버 바인딩
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_sock);
        exit(1);
    }

    // 서버가 클라이언트 연결을 대기하도록 설정
    if (listen(server_sock, 3) == -1) {
        perror("Listen failed");
        close(server_sock);
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // 클라이언트와 연결을 기다림
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1) {
            perror("Connection failed");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        // 클라이언트마다 워커 스레드를 생성하여 통신 처리
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_sock) != 0) {
            perror("Thread creation failed");
        }
    }

    close(server_sock);
    return 0;
}
