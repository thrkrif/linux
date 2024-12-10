#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define NAME_LEN 50

struct data {
    int left_num;
    int right_num;
    char op;
    char name[NAME_LEN];
    int result;
    int min;
    int max;
};

int main() {
    int sock;
    struct sockaddr_in server_addr;
    struct data send_data, received_data;
    struct tm received_time;
    struct sockaddr_in server_info;
    char server_ip[INET_ADDRSTRLEN];
    char formatted_time[100];

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // 서버 IP 설정

    // connect
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    while (1) {
        printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit: ");
        scanf("%d %d %c %s", &send_data.left_num, &send_data.right_num, &send_data.op, send_data.name);

        if (send_data.left_num == 0 && send_data.right_num == 0 && send_data.op == '$' && strcmp(send_data.name, "quit") == 0) {
            printf("Exiting client.\n");
            break;
        }

        // 서버로 데이터 전송
        write(sock, &send_data, sizeof(send_data));

        while (1) {
            // 서버로부터 데이터 수신
            ssize_t read_len = read(sock, &received_data, sizeof(received_data));
            if (read_len <= 0) {
                printf("Server disconnected.\n");
                close(sock);
                exit(EXIT_FAILURE);
            }

            // 서버로부터 시간 데이터 수신
            read(sock, &received_time, sizeof(received_time));

            // 시간 포맷팅
            strftime(formatted_time, sizeof(formatted_time), "%a %b %d %H:%M:%S %Y", &received_time);

            // 서버로부터 IP 정보 수신
            read(sock, &server_info, sizeof(server_info));
            inet_ntop(AF_INET, &server_info.sin_addr, server_ip, INET_ADDRSTRLEN);

            // 출력
            printf("%d %c %d = %d %s min=%d max=%d %s from %s\n",
                   received_data.left_num,
                   received_data.op,
                   received_data.right_num,
                   received_data.result,
                   received_data.name,
                   received_data.min,
                   received_data.max,
                   formatted_time,
                   server_ip);

            sleep(10); // 10초 간격으로 출력 반복
        }
    }

    close(sock);
    return 0;
}
