#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "shared.h"

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    struct client_data client_data;
    struct tm timestamp_data;

    // 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 서버 IP 주소 설정
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }
    printf("Connected to server...\n");

    // 서버의 IP 주소를 문자열로 변환
    char server_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &serv_addr.sin_addr, server_ip, sizeof(server_ip));

    printf("Server IP: %s\n", server_ip);  // 서버 IP 출력

    // 사용자 입력 받기
    printf("Enter input (num1 num2 op name): ");
    scanf("%d %d %c %s", &client_data.left_num, &client_data.right_num, &client_data.op, client_data.name);

    // 서버로 데이터 전송
    send(sock, &client_data, sizeof(client_data), 0);

    // 서버로부터 즉시 데이터 수신
    recv(sock, &client_data, sizeof(client_data), 0);
    recv(sock, &timestamp_data, sizeof(struct tm), 0);

    printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
           client_data.left_num, client_data.op, client_data.right_num, client_data.result,
           client_data.name, client_data.min, client_data.max,
           timestamp_data.tm_hour, timestamp_data.tm_min, timestamp_data.tm_sec, server_ip);

    // 이후 10초 간격으로 데이터 출력
    while (1) {
        recv(sock, &client_data, sizeof(client_data), 0);
        recv(sock, &timestamp_data, sizeof(struct tm), 0);

        printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
               client_data.left_num, client_data.op, client_data.right_num, client_data.result,
               client_data.name, client_data.min, client_data.max,
               timestamp_data.tm_hour, timestamp_data.tm_min, timestamp_data.tm_sec, server_ip);
    }

    close(sock);
    return 0;
}
