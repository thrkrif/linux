#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 3600
#define NAME_LEN 20

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

// SIGCHLD 핸들러: 자식 프로세스 정리
void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    struct sockaddr_in sock_addr, client_addr;
    int listen_sockfd, client_sockfd;
    socklen_t addr_len;
    struct cal_data rdata;

    // 공유 메모리 생성 및 초기화
    int shmid = shmget(IPC_PRIVATE, sizeof(int) * 2, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Shared memory creation failed");
        return 1;
    }
    int *shared_mem = (int *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("Shared memory attachment failed");
        return 1;
    }
    shared_mem[0] = INT_MAX; // min
    shared_mem[1] = INT_MIN; // max

    // SIGCHLD 시그널 처리 등록
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    // 소켓 생성
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(PORT);

    if (bind(listen_sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1) {
        perror("Bind failed");
        return 1;
    }

    if (listen(listen_sockfd, 5) == -1) {
        perror("Listen failed");
        return 1;
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        addr_len = sizeof(client_addr);
        client_sockfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &addr_len);

        if (client_sockfd == -1) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        if (fork() == 0) { // 자식 프로세스 생성
            close(listen_sockfd); // 자식은 listen 소켓 필요 없음

            // 클라이언트로부터 데이터 읽기
            read(client_sockfd, &rdata, sizeof(rdata));

            int left_num = ntohl(rdata.left_num);
            int right_num = ntohl(rdata.right_num);
            char op = rdata.op;
            int cal_result = 0;
            short int cal_error = 0;

            // 계산 수행
            switch (op) {
                case '+': cal_result = left_num + right_num; break;
                case '-': cal_result = left_num - right_num; break;
                case 'x': cal_result = left_num * right_num; break;
                case '/':
                    if (right_num == 0) {
                        cal_error = 2; // Divide by zero error
                    } else {
                        cal_result = left_num / right_num;
                    }
                    break;
                default: cal_error = 1; // Invalid operation error
            }

            // 공유 메모리 갱신
            if (cal_error == 0) {
                cal_result = htonl(cal_result);
                rdata.result = cal_result;
                rdata.error = htons(cal_error);

                // 공유 메모리 접근 및 갱신 (min, max)
                if (cal_result < shared_mem[0]) shared_mem[0] = cal_result;
                if (cal_result > shared_mem[1]) shared_mem[1] = cal_result;

                rdata.min = htonl(shared_mem[0]);
                rdata.max = htonl(shared_mem[1]);
            } else {
                rdata.result = htonl(0);
                rdata.error = htons(cal_error);
                rdata.min = htonl(shared_mem[0]);
                rdata.max = htonl(shared_mem[1]);
            }

            // 클라이언트에 결과 전송
            write(client_sockfd, &rdata, sizeof(rdata));
            close(client_sockfd); // 연결 종료
            exit(0); // 자식 프로세스 종료
        }

        close(client_sockfd); // 부모는 클라이언트 소켓 닫기
    }

    close(listen_sockfd);
    shmdt(shared_mem); // 공유 메모리 분리
    shmctl(shmid, IPC_RMID, NULL); // 공유 메모리 제거

    return 0;
}
