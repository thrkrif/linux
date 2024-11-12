#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>

#define PORT 3600
#define NAME_LEN 20
#define CLIENT_COUNT 3

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
    int listen_sockfd, client_sockfd[CLIENT_COUNT];
    int addr_len;
    struct cal_data rdata;
    int cal_result, min, max;
    short int cal_error;

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

    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        addr_len = sizeof(client_addr);
        client_sockfd[i] = accept(listen_sockfd, (struct sockaddr *)&client_addr, &addr_len);

        if (client_sockfd[i] == -1)
        {
            perror("Accept failed");
            return 1;
        }
        printf("Client %d connected: %s\n", i + 1, inet_ntoa(client_addr.sin_addr));
    }

    min = INT_MAX;
    max = INT_MIN;

    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        read(client_sockfd[i], &rdata, sizeof(rdata));

        cal_error = 0;
        int left_num = ntohl(rdata.left_num);
        int right_num = ntohl(rdata.right_num);

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

        if (cal_error == 0)
        {
            if (cal_result < min)
                min = cal_result;
            if (cal_result > max)
                max = cal_result;
        }

        rdata.result = htonl(cal_result);
        rdata.error = htons(cal_error);
        rdata.min = htonl(min);
        rdata.max = htonl(max);
        write(client_sockfd[i], &rdata, sizeof(rdata));

        printf("%d %c %d = %d %s (min=%d max=%d)\n", left_num, rdata.op, right_num, cal_result, rdata.name, min, max);
        close(client_sockfd[i]);
    }

    close(listen_sockfd);
    return 0;
}
