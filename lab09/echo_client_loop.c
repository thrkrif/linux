#include <sys/socket.h>  /* 소켓 관련 함수 */
#include <arpa/inet.h>   /* 소켓 지원을 위한 각종 함수 */
#include <sys/stat.h>
#include <stdio.h>      /* 표준 입출력 관련 */
#include <string.h>     /* 문자열 관련 */
#include <unistd.h>     /* 각종 시스템 함수 */

#define MAXLINE    1024

int main(int argc, char **argv)
{
    struct sockaddr_in serveraddr;  // 구조체 : 서버 주소 저장
    int server_sockfd;  // 서버 소켓 파일 디스크립터
    int client_len; // 서버 주소 구조체
    char buf[MAXLINE]; // 데이터를 저장할 버퍼

    // 소켓 생성(TCP 소켓)
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("error :");
        return 1;
    }

    /* 연결요청할 서버의 주소와 포트번호 프로토콜등을 지정한다. */
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serveraddr.sin_family = AF_INET;    // ipv4사용 
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버 IP 주소
    serveraddr.sin_port = htons(3600); // 서버 포트 번호

    client_len = sizeof(serveraddr);

    /* 서버에 연결을 시도한다.  connect */
    if (connect(server_sockfd, (struct sockaddr *)&serveraddr, client_len)  == -1)
    {
        perror("connect error :");
        return 1;
    }
 // 클라이언트가 서버와 데이터를 주고받는 반복문
while(1) 
{
    memset(buf, 0x00, MAXLINE); // 버퍼 초기화
    read(0, buf, MAXLINE);    /* 키보드 입력을 기다린다. */
    /* strncmp는 n개의 문자를 비교하는 역할을 한다.
    buf, "quit\n"를 비교한다. 3번째 인자인 n은 비교할 문자열의 최대 길이이다.
    */
    if(strncmp(buf, "quit\n",5) == 0) // 입력이 "quit"이면 연결 종료
   	break;
    if (write(server_sockfd, buf, MAXLINE) <= 0) /* 입력 받은 데이터를 서버로 전송한다. */
    {
        perror("write error : ");
        return 1;
    }

    memset(buf, 0x00, MAXLINE); // 버퍼 초기화
    /* 서버로 부터 데이터를 읽는다. */
    if (read(server_sockfd, buf, MAXLINE) <= 0)
    {
        perror("read error : ");
        return 1;
    }
    printf("read : %s", buf);
}
    close(server_sockfd);
    return 0;
}
