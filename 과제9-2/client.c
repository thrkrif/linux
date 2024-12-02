#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_ADDR "127.0.0.1"

int main() {
    int sock;
    struct sockaddr_in server_addr;
    struct calculation_data calc_data;

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    // 계산 요청 보내기
    calc_data.left_num = 10;
    calc_data.right_num = 5;
    calc_data.op = '+';
    strcpy(calc_data.name, "Client1");
    calc_data.valid_input = 1;
    calc_data.exit_flag = 0;

    send(sock, &calc_data, sizeof(calc_data), 0);

    // 결과 받기
    recv(sock, &calc_data, sizeof(calc_data), 0);
    printf("Calculation result: %d\n", calc_data.result);

    // 연결 종료
    close(sock);
    return 0;
}
