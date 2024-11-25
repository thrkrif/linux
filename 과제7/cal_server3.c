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

#define PORT 3600
#define NAME_LEN 20
#define CLIENT_COUNT 3
// 좀비 프로세스 방지
struct cal_data
{
    int left_num;
    int right_num;
    char op;
    int result;
    short int error;
    char name[NAME_LEN];
    int min;
    int max;
};

int main()
{
    struct sockaddr_in client_addr, sock_addr;
    int listen_sockfd, client_sockfd;
    int addr_len;
    struct cal_data rdata;
    int shmid, *shared_min, *shared_max; // 공유 메모리를 사용하여 모든 자식 프로세스가 동일한 min, max 값을 갱신한다.

    // 공유 메모리 생성 및 초기화
    shmid = shmget(IPC_PRIVATE, sizeof(int) * 2, IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("Shared memory creation failed");
        return 1;
    }

    shared_min = (int *)shmat(shmid, NULL, 0);
    shared_max = shared_min + 1;
    *shared_min = INT_MAX;
    *shared_max = INT_MIN;

    // 소켓 생성 및 바인딩
    if ((listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        return 1;
    }

    memset(&sock_addr, 0x00, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(PORT);

    if (bind(listen_sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1)
    {
        perror("Bind failed");
        return 1;
    }

    if (listen(listen_sockfd, CLIENT_COUNT) == -1)
    {
        perror("Listen failed");
        return 1;
    }

    printf("Server is running...\n");

    // 클라이언트 요청 수락
    while (1)
    {
        addr_len = sizeof(client_addr);
        client_sockfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &addr_len);

        if (client_sockfd == -1)
        {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        if (fork() == 0)
        {
            // 자식 프로세스에서 클라이언트 처리
            close(listen_sockfd);

            read(client_sockfd, &rdata, sizeof(rdata));

            short int cal_error = 0;
            int left_num = ntohl(rdata.left_num);
            int right_num = ntohl(rdata.right_num);
            int cal_result = 0;

            switch (rdata.op)
            {
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
                if (right_num == 0)
                {
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

            // min, max 갱신
            if (cal_error == 0)
            {
                if (cal_result < *shared_min)
                    *shared_min = cal_result;
                if (cal_result > *shared_max)
                    *shared_max = cal_result;
            }

            // 결과 데이터 준비
            rdata.result = htonl(cal_result);
            rdata.error = htons(cal_error);
            rdata.min = htonl(*shared_min);
            rdata.max = htonl(*shared_max);

            write(client_sockfd, &rdata, sizeof(rdata));

            printf("Processed: %d %c %d = %d %s (min=%d max=%d)\n",
                   left_num, rdata.op, right_num, cal_result, rdata.name,
                   *shared_min, *shared_max);

            close(client_sockfd);
            exit(0); // 자식 프로세스 종료
        }

        close(client_sockfd); // 부모는 소켓 닫음

        // 부모가 좀비 프로세스를 방지하기 위해 종료된 자식 프로세스 상태를 수집
        int status;
        while ((waitpid(-1, &status, WNOHANG)) > 0)
        {
            
        }
    }

    close(listen_sockfd);
    shmdt(shared_min);
    shmctl(shmid, IPC_RMID, 0); // 공유 메모리 제거
    return 0;
}
