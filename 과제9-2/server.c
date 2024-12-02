#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <limits.h>

#define PORT 8080
#define MAX_CLIENTS 3

void handle_client(int client_socket, int client_id) {
    int shm_trigger_id = shmget(IPC_PRIVATE, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int shm_producer_id = shmget(IPC_PRIVATE, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int sem_trigger_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    int sem_consumer_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);

    if (shm_trigger_id == -1 || shm_producer_id == -1 || sem_trigger_id == -1 || sem_consumer_id == -1) {
        perror("Failed to create shared memory or semaphore");
        close(client_socket);
        return;
    }

    struct calculation_data *trigger_data = (struct calculation_data *)shmat(shm_trigger_id, NULL, 0);
    struct calculation_data *producer_data = (struct calculation_data *)shmat(shm_producer_id, NULL, 0);

    if (trigger_data == (void *)-1 || producer_data == (void *)-1) {
        perror("Failed to attach shared memory");
        close(client_socket);
        return;
    }

    int min = INT_MAX, max = INT_MIN;

    while (1) {
        sem_wait(sem_trigger_id); // Trigger 신호 대기

        if (trigger_data->exit_flag) {
            printf("Exiting Server...\n");
            producer_data->exit_flag = 1;
            sem_signal(sem_consumer_id); // 종료 신호 전달
            break;
        }

        if (!trigger_data->valid_input) {
            continue; // 유효한 입력이 아니면 대기
        }

        // 연산 처리
        int result = 0;
        switch (trigger_data->op) {
            case '+': result = trigger_data->left_num + trigger_data->right_num; break;
            case '-': result = trigger_data->left_num - trigger_data->right_num; break;
            case 'x': result = trigger_data->left_num * trigger_data->right_num; break;
            case '/': 
                result = (trigger_data->right_num != 0) ? trigger_data->left_num / trigger_data->right_num : 0;
                break;
        }

        // 최소, 최대값 업데이트
        if (result < min) min = result;
        if (result > max) max = result;

        // 데이터 복사 및 업데이트
        memcpy(producer_data, trigger_data, sizeof(struct calculation_data));
        producer_data->result = result;
        producer_data->min = min;
        producer_data->max = max;

        // 시간 정보 저장
        time_t now = time(NULL);
        producer_data->timestamp.timestamp = *localtime(&now);  // timestamp 필드 업데이트

        // Producer IP 주소 설정
        strcpy(producer_data->producer_addr.ip_address, "127.0.0.1");  // 예시로 로컬 IP 사용

    
        printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
               producer_data->left_num, producer_data->op, producer_data->right_num,
               producer_data->result, producer_data->name, producer_data->min, producer_data->max,
               producer_data->timestamp.timestamp.tm_hour, producer_data->timestamp.timestamp.tm_min, producer_data->timestamp.timestamp.tm_sec,
               producer_data->producer_addr.ip_address);

        // Consumer에 데이터 전달
        sem_signal(sem_consumer_id);

        // Producer의 출력 후 대기
        trigger_data->valid_input = 0; // 유효한 입력 처리 후 대기 상태로 변경
    }

    shmdt(trigger_data);
    shmdt(producer_data);
    shmctl(shm_trigger_id, IPC_RMID, NULL);
    shmctl(shm_producer_id, IPC_RMID, NULL);
    semctl(sem_trigger_id, 0, IPC_RMID);
    semctl(sem_consumer_id, 0, IPC_RMID);

    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        if (fork() == 0) {
            handle_client(client_socket, i);
            exit(0);
        }

        close(client_socket); // 부모 프로세스는 소켓 닫음
    }

    close(server_fd);
    return 0;
}
