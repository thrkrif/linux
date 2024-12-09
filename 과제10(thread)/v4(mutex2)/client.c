#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

void *receive_data(void *arg) {
    int sockfd = *(int *)arg;
    char buffer[256];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(sockfd, buffer, sizeof(buffer) - 1);
        if (n <= 0) break;  // 서버 연결 종료 또는 에러 발생 시 종료
        printf("Received from server: %s", buffer);
    }

    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char input[256];
    pthread_t receiver_thread;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to server...\n");

    pthread_create(&receiver_thread, NULL, receive_data, &sockfd);

    while (1) {
        printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit: ");
        fgets(input, sizeof(input), stdin);

        if (strncmp(input, "0 0 $ quit", 11) == 0) {
            break;
        }

        write(sockfd, input, strlen(input));
    }

    pthread_join(receiver_thread, NULL);
    close(sockfd);

    return 0;
}
