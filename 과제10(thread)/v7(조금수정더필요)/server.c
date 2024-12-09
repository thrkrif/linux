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

struct client_info {
    int socket;
    struct sockaddr_in client_addr;
};

void *client_handler(void *arg) {
    struct client_info *info = (struct client_info *)arg;
    int client_sock = info->socket;
    struct sockaddr_in client_addr = info->client_addr;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    struct data received_data;
    struct data send_data = {0, 0, 0, "", 0, INT_MAX, INT_MIN};
    struct tm current_time;

    while (1) {
        // 클라이언트로부터 데이터 수신
        ssize_t recv_len = read(client_sock, &received_data, sizeof(received_data));
        if (recv_len <= 0) {
            printf("Client disconnected: %s\n", client_ip);
            close(client_sock);
            break;
        }

        // 계산 수행
        if (received_data.op == '+') {
            send_data.result = received_data.left_num + received_data.right_num;
        } else if (received_data.op == '-') {
            send_data.result = received_data.left_num - received_data.right_num;
        } else if (received_data.op == 'x') {
            send_data.result = received_data.left_num * received_data.right_num;
        } else if (received_data.op == '/') {
            send_data.result = received_data.left_num / received_data.right_num;
        }

        // min, max 업데이트
        if (send_data.result < send_data.min) {
            send_data.min = send_data.result;
        }
        if (send_data.result > send_data.max) {
            send_data.max = send_data.result;
        }

        // 이름, 숫자 정보 복사
        strcpy(send_data.name, received_data.name);
        send_data.left_num = received_data.left_num;
        send_data.right_num = received_data.right_num;
        send_data.op = received_data.op;

        // 현재 시간
        time_t now = time(NULL);
        localtime_r(&now, &current_time);

        // 결과 전송
        printf("Sending data to client %s\n", client_ip);
        write(client_sock, &send_data, sizeof(send_data));
        write(client_sock, &current_time, sizeof(current_time));
        write(client_sock, &client_addr, sizeof(client_addr));

        // 10초 대기
        sleep(10);
    }

    free(info);
    pthread_exit(NULL);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 바인딩
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 리스닝
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        // accept
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        struct client_info *info = malloc(sizeof(struct client_info));
        info->socket = client_sock;
        info->client_addr = client_addr;

        // new worker thread : client가 데이터 요청(write()) 하면 여기서 
        // read()하고 write()하여 client에게 데이터를 수신한다(read())
        pthread_create(&tid, NULL, client_handler, info);
        pthread_detach(tid); // main 스레드로부터 분리하여 worker 스레드의 종료를 기다리지 않도록 한다.
    }

    close(server_sock);
    return 0;
}
