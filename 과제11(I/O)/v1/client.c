// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 12345
#define BUFFER_SIZE 1024

struct ClientData {
    int num1;
    int num2;
    char operator;
    char message[BUFFER_SIZE];
};

struct ServerResponse {
    int result;
    int min;
    int max;
    struct tm timestamp;
    struct sockaddr_in client_addr;
};

int main() {
    int sock;
    struct sockaddr_in server_addr;
    struct ClientData data;
    struct ServerResponse response;

    // 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Enter data (format: num1 num2 operator message):\n");

    while (1) {
        // 입력받기
        printf("Input: ");
        scanf("%d %d %c %s", &data.num1, &data.num2, &data.operator, data.message);

        // 데이터 전송
        if (send(sock, &data, sizeof(data), 0) < 0) {
            perror("Send failed");
            break;
        }

        // 종료 조건
        if (data.num1 == 0 && data.num2 == 0 && data.operator == '$') {
            printf("Exiting...\n");
            break;
        }

        // 응답 수신
        if (recv(sock, &response, sizeof(response), 0) < 0) {
            perror("Receive failed");
            break;
        }

         // 결과 출력 (ClientData와 ServerResponse를 함께 사용)
        char time_buffer[BUFFER_SIZE];
        strftime(time_buffer, sizeof(time_buffer), "%a %b %d %H:%M:%S %Y", &response.timestamp);

        printf("%d%c%d=%d %s min=%d max=%d %s from %s\n",
            data.num1,                       
            data.operator,                   
            data.num2,                       
            response.result,                 
            data.message,                    
            response.min,                    
            response.max,                    
            time_buffer,                     
            inet_ntoa(response.client_addr.sin_addr)); // 서버에서 반환된 클라이언트 IP
    
    }

    close(sock);
    return 0;
}
