#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "shared.h" 

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    struct client_data client_data;
    struct timestamp_data timestamp_data;
    struct server_info server_info;

    // 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 서버 주소 설정
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

    // 클라이언트 입력 받기
    printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit: ");
    scanf("%d %d %c %s", &client_data.left_num, &client_data.right_num, &client_data.op, client_data.name);

    // 서버에 데이터 전송
    send(sock, &client_data, sizeof(client_data), 0);

    // 서버로부터 데이터 수신
    recv(sock, &client_data, sizeof(client_data), 0);
    recv(sock, &timestamp_data, sizeof(timestamp_data), 0);
    recv(sock, &server_info, sizeof(server_info), 0);

    // 결과 출력
    printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
           client_data.left_num, client_data.op, client_data.right_num, client_data.result,
           client_data.name, server_info.min, server_info.max,
           timestamp_data.current_time.tm_hour, timestamp_data.current_time.tm_min, timestamp_data.current_time.tm_sec,
           inet_ntoa(server_info.server_addr.sin_addr));

    // 10초 간격으로 반복 출력
    while (1) {
        sleep(10);
        printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
               client_data.left_num, client_data.op, client_data.right_num, client_data.result,
               client_data.name, server_info.min, server_info.max,
               timestamp_data.current_time.tm_hour, timestamp_data.current_time.tm_min, timestamp_data.current_time.tm_sec,
               inet_ntoa(server_info.server_addr.sin_addr));
    }

    close(sock);
    return 0;
}
