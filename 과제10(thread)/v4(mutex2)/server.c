#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 3

pthread_mutex_t t_lock;
pthread_cond_t t_cond;

struct client_data {
    int num1;
    int num2;
    char op;
    char name[50];
};

struct timestamp_data {
    time_t last_time;
};

struct server_info {
    struct sockaddr_in server_addr;
};

struct shared_data {
    int min;
    int max;
    struct client_data data[MAX_CLIENTS];
    struct timestamp_data timestamp;
    struct server_info server_address;
};

struct shared_data shared_data;

void *client_handler(void *arg) {
    int client_id = *(int *)arg;
    struct client_data *client = &shared_data.data[client_id];
    int result = 0;

    // 연산 수행
    if (client->op == '+') {
        result = client->num1 + client->num2;
    } else if (client->op == '-') {
        result = client->num1 - client->num2;
    } else if (client->op == 'x') {
        result = client->num1 * client->num2;
    } else if (client->op == '/') {
        result = client->num1 / client->num2;
    }

    pthread_mutex_lock(&t_lock);  // 공유 자원 보호

    // min, max 업데이트
    if (result < shared_data.min) shared_data.min = result;
    if (result > shared_data.max) shared_data.max = result;

    // 시간 업데이트
    shared_data.timestamp.last_time = time(NULL);

    // 서버 IP 주소 출력
    printf("%d%c%d=%d %s min=%d max=%d %s from %s\n",
           client->num1, client->op, client->num2, result, client->name,
           shared_data.min, shared_data.max, ctime(&shared_data.timestamp.last_time),
           inet_ntoa(shared_data.server_address.server_addr.sin_addr));

    pthread_mutex_unlock(&t_lock);  // 공유 자원 해제

    return NULL;
}

int main() {
    pthread_t threads[MAX_CLIENTS];
    int client_ids[MAX_CLIENTS];
    int client_count = 0;
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[256];

    // 초기화
    shared_data.min = 0;
    shared_data.max = 0;
    shared_data.timestamp.last_time = time(NULL);

    pthread_mutex_init(&t_lock, NULL);
    pthread_cond_init(&t_cond, NULL);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        return 1;
    }

    listen(server_sock, MAX_CLIENTS);
    printf("Server listening on %s:%d...\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    // 서버 주소 저장
    shared_data.server_address.server_addr = server_addr;

    while (1) {
        client_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // 클라이언트 데이터 받기
        if (client_count < MAX_CLIENTS) {
            int n = read(client_sock, buffer, sizeof(buffer));
            if (n > 0) {
                struct client_data client;
                sscanf(buffer, "%d %d %c %s", &client.num1, &client.num2, &client.op, client.name);
                shared_data.data[client_count] = client;
                client_ids[client_count] = client_count;
                pthread_create(&threads[client_count], NULL, client_handler, &client_ids[client_count]);
                client_count++;
            }
        }

        // 모든 클라이언트 처리가 끝나면 대기
        if (client_count == MAX_CLIENTS) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                pthread_join(threads[i], NULL);
            }
            client_count = 0;  // 클라이언트 수 초기화
        }

        close(client_sock);  // 클라이언트 연결 종료
    }

    pthread_mutex_destroy(&t_lock);
    pthread_cond_destroy(&t_cond);
    close(server_sock);  // 서버 소켓 종료

    return 0;
}
