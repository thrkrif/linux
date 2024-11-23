#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 3600
#define IP "127.0.0.1"
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

int main() {
    struct sockaddr_in addr;
    int s, len, sbyte, rbyte;
    struct cal_data sdata;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1) {
        perror("Socket creation failed");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("Fail to connect\n");
        close(s);
        return 1;
    }

    printf("Connected to server. Enter calculation in format [num1] [num2] [op] [name]:\n");

    printf("Input: ");
    scanf("%d %d %c %s", &sdata.left_num, &sdata.right_num, &sdata.op, sdata.name);

    sdata.left_num = htonl(sdata.left_num);
    sdata.right_num = htonl(sdata.right_num);

    sbyte = write(s, &sdata, sizeof(sdata));
    if (sbyte != sizeof(sdata)) {
        perror("Write failed");
        close(s);
        return 1;
    }

    rbyte = read(s, &sdata, sizeof(sdata));
    if (rbyte != sizeof(sdata)) {
        perror("Read failed");
        close(s);
        return 1;
    }

    if (ntohs(sdata.error) != 0) {
        printf("CALC Error %d\n", ntohs(sdata.error));
    } else {
        printf("%d %c %d = %d %s (min=%d max=%d)\n", 
            ntohl(sdata.left_num), 
            sdata.op, 
            ntohl(sdata.right_num), 
            ntohl(sdata.result), 
            sdata.name, 
            ntohl(sdata.min), 
            ntohl(sdata.max));
    }

    close(s);
    return 0;
}
