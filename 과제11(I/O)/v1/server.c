// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>

#define PORT 12345
#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024

struct ClientData {
    int num1;
    int num2;
    char operator;
    char message[BUFFER_SIZE];
};

struct ServerResponse {
    int result;
    int min;
    int max;
    struct tm timestamp;
    struct sockaddr_in client_addr;
};

void handle_client(int client_sock, struct sockaddr_in *client_addr, int *min, int *max, int *client_sockets, int index) {
    struct ClientData data;
    struct ServerResponse response;
    
    if (recv(client_sock, &data, sizeof(data), 0) <= 0) {
        perror("recv");
        close(client_sock);
        client_sockets[index] = 0; // 배열에서 소켓 제거
        return;
    }

    // 계산 처리
    switch (data.operator) {
        case '+': response.result = data.num1 + data.num2; break;
        case '-': response.result = data.num1 - data.num2; break;
        case 'x': response.result = data.num1 * data.num2; break;
        case '/':
            if (data.num2 != 0) response.result = data.num1 / data.num2;
            else response.result = 0;
            break;
        default: response.result = 0; break;
    }

    // min, max 업데이트
    if (*min == -1 || response.result < *min) *min = response.result;
    if (*max == -1 || response.result > *max) *max = response.result;

    response.min = *min;
    response.max = *max;
    response.timestamp = *localtime(&(time_t){time(NULL)});
    response.client_addr = *client_addr;

    if (send(client_sock, &response, sizeof(response), 0) <= 0) {
        perror("send");
    }

    if (data.num1 == 0 && data.num2 == 0 && data.operator == '$' && strcmp(data.message, "quit") == 0) {
        printf("Client disconnected.\n");
        client_sockets[index] = 0; // 배열에서 소켓 제거
        close(client_sock);
    }
}

int main() {
    int server_sock, client_sock, max_sd, activity;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    fd_set readfds;
    int client_sockets[MAX_CLIENTS] = {0};
    int min = -1, max = -1;

    // 소켓 생성
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        max_sd = server_sock;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &readfds);
            }

            if (client_sockets[i] > max_sd) {
                max_sd = client_sockets[i];
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        if (FD_ISSET(server_sock, &readfds)) {
            if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("New connection: socket fd is %d, ip is %s, port %d\n",
                   client_sock, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_sock;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(client_sockets[i], &readfds)) {
                handle_client(client_sockets[i], &client_addr, &min, &max, client_sockets, i);
            }
        }
    }
    return 0;
}
