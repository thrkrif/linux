#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <limits.h>
#include "shared.h" 

#define PORT 8080

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address, client_addr;
    int opt = 1;
    int addrlen = sizeof(address);

    struct client_data client_data;
    struct timestamp_data timestamp_data;
    struct server_info server_info = {.min = INT_MAX, .max = INT_MIN};

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

    // 소켓 바인딩
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 대기 시작
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d...\n", PORT);

    // 클라이언트 접속 처리
    while ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen)) >= 0) {
        // 데이터 수신
        recv(new_socket, &client_data, sizeof(client_data), 0);

        // 연산 결과 계산
        switch (client_data.op) {
        case '+':
            client_data.result = client_data.left_num + client_data.right_num;
            break;
        case '-':
            client_data.result = client_data.left_num - client_data.right_num;
            break;
        case 'x':
            client_data.result = client_data.left_num * client_data.right_num;
            break;
        case '/':
            client_data.result = client_data.right_num != 0 ? client_data.left_num / client_data.right_num : 0;
            break;
        default:
            client_data.result = 0;
            break;
        }

        // 최소, 최대값 업데이트
        if (client_data.result < server_info.min)
            server_info.min = client_data.result;
        if (client_data.result > server_info.max)
            server_info.max = client_data.result;

        // 시간 정보 갱신
        time_t now = time(NULL);
        timestamp_data.current_time = *localtime(&now);

        // 서버 주소 저장
        server_info.server_addr = address;

        // 데이터 전송
        send(new_socket, &client_data, sizeof(client_data), 0);
        send(new_socket, &timestamp_data, sizeof(timestamp_data), 0);
        send(new_socket, &server_info, sizeof(server_info), 0);
        close(new_socket);
    }

    return 0;
}
