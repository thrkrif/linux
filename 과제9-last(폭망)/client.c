#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/sem.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_SIZE 1024
#define SEM_KEY 1234
#define SHM_KEY 5678

// 연산 데이터를 위한 구조체
struct calculation_data {
    int num1;
    int num2;
    char operator;
    char str[100];
    int result;
    int min;
    int max;
};

// 시간 데이터를 위한 구조체
struct timestamp_data {
    struct tm timestamp;
};

// 클라이언트 정보 (IP) 구조체
struct client_info {
    char client_ip[INET_ADDRSTRLEN];
};

void wait_semaphore(int sem_id) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

void signal_semaphore(int sem_id) {
    struct sembuf op = {1, 1, 0};
    semop(sem_id, &op, 1);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    struct calculation_data calculation;
    struct timestamp_data timestamp;
    struct client_info client_info;

    // 세마포어 생성
    int sem_id = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (sem_id < 0) {
        perror("Semaphore creation failed");
        exit(1);
    }

    // 서버와 연결
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    
    while (1) {
        // 사용자로부터 입력 받기
        printf("Enter calculation (num1 num2 operator str): ");
        scanf("%d %d %c %s", &calculation.num1, &calculation.num2, &calculation.operator, calculation.str);
        
        // 계산 결과 보내기
        send(sock, &calculation, sizeof(struct calculation_data), 0);
        
        // "quit" 입력 시 종료
        if (strcmp(calculation.str, "quit") == 0) {
            break;
        }

        // 서버로부터 결과 받기
        wait_semaphore(sem_id); // 세마포어 대기
        recv(sock, &calculation, sizeof(struct calculation_data), 0);
        recv(sock, &timestamp, sizeof(struct timestamp_data), 0);
        recv(sock, &client_info, sizeof(struct client_info), 0);
        signal_semaphore(sem_id); // 세마포어 신호

        // 결과 출력
        printf("Result: %d %s min=%d max=%d %s from %s\n",
               calculation.result, calculation.str, calculation.min, calculation.max, asctime(&timestamp.timestamp), client_info.client_ip);
    }

    close(sock);
    return 0;
}
