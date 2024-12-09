#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

// 구조체 정의
typedef struct {
    int num1;
    int num2;
    char operator;
    char str[BUF_SIZE];
} ClientData;

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    ClientData client_data;

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    // 클라이언트 입력 받기
    while (1) {
        printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit:");
        fgets(buffer, BUF_SIZE, stdin);

        // 사용자 입력 처리
        if (sscanf(buffer, "%d %d %c %s", &client_data.num1, &client_data.num2, &client_data.operator, client_data.str) != 4) {
            printf("Invalid input.\n");
            continue;
        }

        // "quit" 입력 시 종료
        if (strcmp(client_data.str, "quit") == 0) {
            printf("Exiting...\n");
            break;
        }

        // 서버로 데이터 전송
        send(sock, &client_data, sizeof(client_data), 0);

        // 서버로부터 계산된 결과 받기
        recv(sock, buffer, BUF_SIZE, 0);
        printf("Received from server: %s\n", buffer);
    }

    close(sock);
    return 0;
}
