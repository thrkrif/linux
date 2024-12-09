#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define BUF_SIZE 1024

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

int main() {
    int sock;
    struct sockaddr_in server_addr;
    ClientData client_data;
    CalculationTime calc_time;
    ServerInfo server_info;

    // 서버 주소 설정
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버 IP 주소

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to server...\n");

    while (1) {
        // 서버로부터 3개의 구조체 수신
        recv(sock, &client_data, sizeof(ClientData), 0);
        recv(sock, &calc_time, sizeof(CalculationTime), 0);
        recv(sock, &server_info, sizeof(ServerInfo), 0);

        // 수신한 데이터를 출력
        printf("%d%c%d=%d %s min=%d max=%d %s from %s\n",
            client_data.num1, client_data.operator, client_data.num2, client_data.min, 
            client_data.str, client_data.min, client_data.max, 
            asctime(&calc_time.timestamp), inet_ntoa(server_info.client_addr.sin_addr));

        // "quit" 문자열이 들어오면 종료
        if (strcmp(client_data.str, "quit") == 0) {
            printf("Server has sent quit, closing connection...\n");
            break;
        }

        // 10초 대기 후 계속 수신
        sleep(10);
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
