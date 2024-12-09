#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 12345
#define BUF_SIZE 1024
#define MAX_CLIENTS 3

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
} CalcTimeData;

typedef struct {
    struct sockaddr_in client_addr; // 클라이언트의 주소
} ClientAddrData;

// 클라이언트와의 연결을 처리하는 함수
void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    ClientData client_data;
    CalcTimeData calc_time_data;
    ClientAddrData client_addr_data;

    // min, max 초기화
    int min = 999999, max = -999999;

    while (1) {
        // 클라이언트로부터 데이터 받기
        ssize_t bytes_received = recv(client_sock, &client_data, sizeof(ClientData), 0);
        if (bytes_received <= 0) {
            break; // 클라이언트가 종료되었거나 오류 발생
        }

        // 계산 수행
        int result;
        if (client_data.operator == '+') {
            result = client_data.num1 + client_data.num2;
        } else if (client_data.operator == '-') {
            result = client_data.num1 - client_data.num2;
        } else if (client_data.operator == '*') {
            result = client_data.num1 * client_data.num2;
        } else if (client_data.operator == '/') {
            if (client_data.num2 != 0) {
                result = client_data.num1 / client_data.num2;
            } else {
                result = 0; // 0으로 나누기 방지
            }
        }

        // min, max 갱신
        if (result < min) min = result;
        if (result > max) max = result;
        client_data.min = min;
        client_data.max = max;

        // 현재 시간 가져오기
        time_t current_time = time(NULL);
        calc_time_data.timestamp = *localtime(&current_time);

        // 클라이언트의 IP 주소 정보
        client_addr_data.client_addr = *(struct sockaddr_in*)arg;

        // 10초 간격으로 클라이언트에 응답 보내기
        while (1) {
            // 클라이언트로 데이터 보내기
            send(client_sock, &client_data, sizeof(ClientData), 0);
            send(client_sock, &calc_time_data, sizeof(CalcTimeData), 0);
            send(client_sock, &client_addr_data, sizeof(ClientAddrData), 0);

            sleep(10); // 10초 간격으로 보내기
        }
    }

    close(client_sock);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    pthread_t threads[MAX_CLIENTS];

    // 서버 소켓 생성
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_sock, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // 클라이언트 연결 처리
    int i = 0;
    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size)) == -1) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        // 클라이언트마다 워커 스레드 생성
        if (pthread_create(&threads[i], NULL, handle_client, &client_sock) != 0) {
            perror("Thread creation failed");
        }
        i++;

        if (i >= MAX_CLIENTS) break;  // 최대 클라이언트 수 처리
    }

    close(server_sock);
    return 0;
}
