#include "shared.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

void producer(int socket_fd) {
    struct calculation_data data = {0};
    while (1) {
        printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit: ");
        scanf("%d %d %c %s", &data.left_num, &data.right_num, &data.op, data.name);

        if (data.left_num == 0 && data.right_num == 0 && data.op == '$') {
            data.exit_flag = 1;
            send(socket_fd, &data, sizeof(data), 0);
            break;
        }

        data.valid_input = 1;
        send(socket_fd, &data, sizeof(data), 0);
    }
}

void consumer(int socket_fd) {
    struct calculation_data data = {0};
    while (1) {
        recv(socket_fd, &data, sizeof(data), 0);

        if (data.exit_flag) {
            printf("Server closed connection.\n");
            break;
        }

        printf("%d %c %d = %d %s min=%d max=%d\n",
               data.left_num, data.op, data.right_num, data.result,
               data.name, data.min, data.max);
    }
}

int main() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
    };

    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    if (fork() == 0) {
        producer(socket_fd);
        exit(0);
    } else {
        consumer(socket_fd);
    }

    close(socket_fd);
    return 0;
}