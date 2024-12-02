#include "shared.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits.h>

#define PORT 8080
#define MAX_CLIENTS 3

void handle_client(int client_socket, int client_id) {
    int shm_id = shmget(IPC_PRIVATE, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);

    if (shm_id == -1 || sem_id == -1) {
        perror("Failed to create shared memory or semaphore");
        close(client_socket);
        return;
    }

    semctl(sem_id, 0, SETVAL, 0); // 세마포어 초기화
    struct calculation_data *data = (struct calculation_data *)shmat(shm_id, NULL, 0);

    if (data == (void *)-1) {
        perror("Failed to attach shared memory");
        close(client_socket);
        return;
    }

    // 서버 정보 구조체
    struct server_info_data info_data;
    // 현재 서버의 IP 주소를 가져오기
    strcpy(info_data.ip_address, "127.0.0.1"); // 서버의 IP 주소 (추후 동적 IP 획득 가능)

    int min = INT_MAX, max = INT_MIN;

    while (1) {
        recv(client_socket, data, sizeof(struct calculation_data), 0);

        if (data->exit_flag) {
            printf("Client %d disconnected.\n", client_id);
            break;
        }

        // 계산 수행
        int result = 0;
        switch (data->op) {
            case '+': result = data->left_num + data->right_num; break;
            case '-': result = data->left_num - data->right_num; break;
            case 'x': result = data->left_num * data->right_num; break;
            case '/': result = data->right_num != 0 ? data->left_num / data->right_num : 0; break;
            default: result = 0; break;
        }

        data->result = result;
        if (result < min) min = result;
        if (result > max) max = result;

        data->min = min;
        data->max = max;

        // 계산 결과와 서버 주소 전송
        send(client_socket, data, sizeof(struct calculation_data), 0);
        send(client_socket, &info_data, sizeof(info_data), 0);  // 서버 IP 주소도 함께 전송
    }

    shmdt(data);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
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
