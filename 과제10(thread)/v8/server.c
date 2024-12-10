#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_CLIENTS 3
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

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
struct data shared_data;
int data_available = 0;

struct client_info {
    int socket;
    struct sockaddr_in client_addr;
};

void *producer(void *arg) {
    int client_sock = *((int *)arg);
    struct data received_data;

    while (1) {
        ssize_t recv_len = read(client_sock, &received_data, sizeof(received_data));
        if (recv_len <= 0) {
            printf("Client disconnected.\n");
            close(client_sock);
            break;
        }

        // 로그 추가: 클라이언트로부터 받은 데이터 출력
        printf("Received data from client: %d %c %d, Name: %s\n",
            received_data.left_num, received_data.op, received_data.right_num, received_data.name);
            
        pthread_mutex_lock(&lock);
        shared_data = received_data;
        data_available = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(NULL);
}

void *consumer(void *arg) {
    int client_sock = *((int *)arg);
    struct data send_data = {0, 0, 0, "", 0, INT_MAX, INT_MIN};
    struct tm current_time;

    while (1) {
        pthread_mutex_lock(&lock);
        while (!data_available) {
            pthread_cond_wait(&cond, &lock);
        }

        send_data = shared_data;
        data_available = 0;
        pthread_mutex_unlock(&lock);

        if (send_data.op == '+') {
            send_data.result = send_data.left_num + send_data.right_num;
        } else if (send_data.op == '-') {
            send_data.result = send_data.left_num - send_data.right_num;
        } else if (send_data.op == 'x') {
            send_data.result = send_data.left_num * send_data.right_num;
        } else if (send_data.op == '/') {
            send_data.result = send_data.left_num / send_data.right_num;
        }

        if (send_data.result < send_data.min) send_data.min = send_data.result;
        if (send_data.result > send_data.max) send_data.max = send_data.result;

         // 로그 추가: 클라이언트로 보낼 데이터 출력
        printf("Sending data to client: %d %c %d = %d, Min: %d, Max: %d\n",
            send_data.left_num, send_data.op, send_data.right_num, send_data.result, send_data.min, send_data.max);

        time_t now = time(NULL);
        localtime_r(&now, &current_time);

        write(client_sock, &send_data, sizeof(send_data));
        write(client_sock, &current_time, sizeof(current_time));
        sleep(10);
    }
    pthread_exit(NULL);
}

void *client_handler(void *arg) {
    struct client_info *info = (struct client_info *)arg;
    int client_sock = info->socket;

    pthread_t producer_thread, consumer_thread;

    // 생산자 스레드 생성
    pthread_create(&producer_thread, NULL, producer, &client_sock);
    pthread_detach(producer_thread); // 생산자 스레드 분리

    // 소비자 스레드 생성
    pthread_create(&consumer_thread, NULL, consumer, &client_sock);
    pthread_detach(consumer_thread); // 소비자 스레드 분리

    free(info);
    pthread_exit(NULL);
}


int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        struct client_info *info = malloc(sizeof(struct client_info));
        info->socket = client_sock;
        info->client_addr = client_addr;

        pthread_create(&tid, NULL, client_handler, info);
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}
