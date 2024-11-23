#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <pthread.h>

#define PORT 3600
#define NAME_LEN 20
#define CLIENT_COUNT 3

struct cal_data {
    int left_num;
    int right_num;
    char op;
    int result;
    short int error;
    char name[NAME_LEN];
    int min;
    int max;
};

int min = INT_MAX, max = INT_MIN;
pthread_mutex_t lock;

void *handle_client(void *arg) {
    int client_sockfd = *(int *)arg;
    struct cal_data rdata;
    int cal_result;
    short int cal_error;

    while (1) {
        memset(&rdata, 0x00, sizeof(rdata));
        if (read(client_sockfd, &rdata, sizeof(rdata)) <= 0) {
            printf("Client disconnected.\n");
            close(client_sockfd);
            return NULL;
        }

        cal_error = 0;
        int left_num = ntohl(rdata.left_num);
        int right_num = ntohl(rdata.right_num);

        switch (rdata.op) {
            case '+':
                cal_result = left_num + right_num;
                break;
            case '-':
                cal_result = left_num - right_num;
                break;
            case 'x':
                cal_result = left_num * right_num;
                break;
            case '/':
                if (right_num == 0) {
                    cal_error = 2;
                    cal_result = 0;
                    break;
                }
                cal_result = left_num / right_num;
                break;
            default:
                cal_error = 1;
                cal_result = 0;
        }

        pthread_mutex_lock(&lock);
        if (cal_error == 0) {
            if (cal_result < min)
                min = cal_result;
            if (cal_result > max)
                max = cal_result;
        }
        pthread_mutex_unlock(&lock);

        rdata.result = htonl(cal_result);
        rdata.error = htons(cal_error);
        rdata.min = htonl(min);
        rdata.max = htonl(max);

        write(client_sockfd, &rdata, sizeof(rdata));
        printf("%d %c %d = %d %s (min=%d max=%d)\n", left_num, rdata.op, right_num, cal_result, rdata.name, min, max);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    int listen_sockfd, client_sockfd[CLIENT_COUNT];
    int addr_len;
    pthread_t threads[CLIENT_COUNT];

    pthread_mutex_init(&lock, NULL);

    if ((listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&server_addr, 0x00, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(listen_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        return 1;
    }

    if (listen(listen_sockfd, CLIENT_COUNT) == -1) {
        perror("Listen failed");
        return 1;
    }

    printf("Server is listening on port %d...\n", PORT);

    for (int i = 0; i < CLIENT_COUNT; i++) {
        addr_len = sizeof(client_addr);
        client_sockfd[i] = accept(listen_sockfd, (struct sockaddr *)&client_addr, &addr_len);

        if (client_sockfd[i] == -1) {
            perror("Accept failed");
            return 1;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
        pthread_create(&threads[i], NULL, handle_client, &client_sockfd[i]);
    }

    for (int i = 0; i < CLIENT_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    close(listen_sockfd);
    pthread_mutex_destroy(&lock);
    return 0;
}
