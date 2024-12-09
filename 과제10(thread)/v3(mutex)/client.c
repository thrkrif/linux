#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 12345
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
} CalcTimeData;

typedef struct {
    struct sockaddr_in client_addr; // 클라이언트의 주소
} ClientAddrData;

int main() {
    int sock;
    struct sockaddr_in server_addr;
    ClientData client_data;
    CalcTimeData calc_time_data;
    ClientAddrData client_addr_data;

    // 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 서버 IP 주소 (로컬호스트)

    // 서버에 연결
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect failed");
        exit(1);
    }

    printf("Connected to server...\n");

    while (1) {
        // 사용자 입력 받기
        printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit: ");
        scanf("%d %d %c %s", &client_data.num1, &client_data.num2, &client_data.operator, client_data.str);

        // 종료 조건
        if (client_data.num1 == 0 && client_data.num2 == 0 && client_data.operator == '$') {
            break;
        }

        // 서버에 데이터 전송
        send(sock, &client_data, sizeof(client_data), 0);

        // 서버로부터 결과 받기
        while (1) {
            ssize_t bytes_received = recv(sock, &client_data, sizeof(client_data), 0);
            if (bytes_received <= 0) {
                break;
            }
            recv(sock, &calc_time_data, sizeof(calc_time_data), 0);
            recv(sock, &client_addr_data, sizeof(client_addr_data), 0);

            // 받은 데이터 출력
            printf("%d%c%d=%d %s min=%d max=%d %s from %s\n",
                   client_data.num1, client_data.operator, client_data.num2,
                   client_data.min, client_data.str, client_data.min, client_data.max,
                   asctime(&calc_time_data.timestamp), inet_ntoa(client_addr_data.client_addr.sin_addr));
        }

        sleep(10);  // 10초 대기
    }

    close(sock);
    return 0;
}
