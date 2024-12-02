#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 3
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

// 세마포어 초기화
void init_semaphore(int sem_id) {
    semctl(sem_id, 0, SETVAL, 1);
    semctl(sem_id, 1, SETVAL, 0);
}

// 세마포어 연산
void wait_semaphore(int sem_id) {
    struct sembuf op = {0, -1, 0}; // P 연산
    semop(sem_id, &op, 1);
}

void signal_semaphore(int sem_id) {
    struct sembuf op = {1, 1, 0}; // V 연산
    semop(sem_id, &op, 1);
}

// 클라이언트 처리
void handle_client(int client_sock, int shmid, int sem_id) {
    struct calculation_data *calculation = (struct calculation_data*) shmat(shmid, NULL, 0);
    struct timestamp_data *timestamp = (struct timestamp_data*) shmat(shmid, NULL, 0);
    struct client_info *client_info = (struct client_info*) shmat(shmid, NULL, 0);
    
    while (1) {
        // 클라이언트로부터 데이터 수신
        recv(client_sock, calculation, sizeof(struct calculation_data), 0);
        
        // 종료 조건 체크
        if (calculation->num1 == 0 && calculation->num2 == 0 && calculation->operator == '$' && strcmp(calculation->str, "quit") == 0) {
            break;
        }
        
        // 연산 수행
        switch (calculation->operator) {
            case '+':
                calculation->result = calculation->num1 + calculation->num2;
                break;
            case '-':
                calculation->result = calculation->num1 - calculation->num2;
                break;
            case 'x':
                calculation->result = calculation->num1 * calculation->num2;
                break;
            case '/':
                if (calculation->num2 != 0) {
                    calculation->result = calculation->num1 / calculation->num2;
                } else {
                    calculation->result = 0;
                }
                break;
        }
        
        // 최소값, 최대값 계산
        if (calculation->result < calculation->min) calculation->min = calculation->result;
        if (calculation->result > calculation->max) calculation->max = calculation->result;
        
        // 현재 시간 기록
        time_t t = time(NULL);
        timestamp->timestamp = *localtime(&t);
        
        // 클라이언트 IP 주소 저장
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        getpeername(client_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        inet_ntop(AF_INET, &client_addr.sin_addr, client_info->client_ip, INET_ADDRSTRLEN);
        
        // 공유 메모리에 데이터 기록
        wait_semaphore(sem_id);
        
        // 10초 간격으로 클라이언트에 결과 전송
        send(client_sock, calculation, sizeof(struct calculation_data), 0);
        send(client_sock, timestamp, sizeof(struct timestamp_data), 0);
        send(client_sock, client_info, sizeof(struct client_info), 0);
        
        signal_semaphore(sem_id);
        sleep(10);
    }
    
    // 연결 종료
    close(client_sock);
}

// 서버 main 함수
int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    
    // 소켓 바인딩
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // 클라이언트 연결 대기
    listen(server_sock, 3);
    
    // 공유 메모리 생성
    int shmid = shmget(SHM_KEY, sizeof(struct calculation_data) + sizeof(struct timestamp_data) + sizeof(struct client_info), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Shared memory creation failed");
        exit(1);
    }
    
    // 세마포어 생성
    int sem_id = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (sem_id < 0) {
        perror("Semaphore creation failed");
        exit(1);
    }
    
    init_semaphore(sem_id);
    
    // 클라이언트 처리
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }
        
        if (fork() == 0) {
            // 자식 프로세스에서 클라이언트 처리
            close(server_sock);
            handle_client(client_sock, shmid, sem_id);
            exit(0);
        } else {
            // 부모 프로세스는 클라이언트 처리 계속
            close(client_sock);
        }
    }
    
    return 0;
}
