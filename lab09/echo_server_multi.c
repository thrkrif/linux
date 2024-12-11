#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>

#define MAXLINE 1024
#define PORTNUM 3600
#define SOCK_SETSIZE 1021

int main(int argc, char **argv)
{
	 int listen_fd, client_fd;          // 리슨 소켓과 클라이언트 소켓 파일 디스크립터
    socklen_t addrlen;                // 클라이언트 주소 크기
    int fd_num;                       // select()에서 이벤트가 발생한 소켓 수
    int maxfd = 0;                    // 가장 큰 파일 디스크립터 값
    int sockfd;                       // 현재 처리 중인 소켓 파일 디스크립터
    int i = 0;                        // 반복문 인덱스
    char buf[MAXLINE];                // 데이터를 저장할 버퍼
    fd_set readfds, allfds;           // 읽기 소켓 집합과 전체 소켓 집합

	struct sockaddr_in server_addr, client_addr; // 서버와 클라이언트 주소 구조체

	// 소켓 생성 (TCP 소켓), listen_fd : 서버소켓 파일 디스크립터
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket error");
		return 1;
	}   
	// 서버 주소 구조체 초기화 및 설정
	memset((void *)&server_addr, 0x00, sizeof(server_addr)); // 구조체 초기화
	server_addr.sin_family = AF_INET; // IPv4 사용
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 IP에서 접속 허용
	server_addr.sin_port = htons(PORTNUM);// 포트 번호 설정
	
	// 소켓에 주소 바인딩
	if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind error");
		return 1;
	}   
	// listen (클라이언트 연결 대기)
	if(listen(listen_fd, 5) == -1)
	{
		perror("listen error");
		return 1;
	}   
	
	FD_ZERO(&readfds); // 읽기 소켓 집합 초기화
    FD_SET(listen_fd, &readfds);   // 리슨 소켓 추가

	maxfd = listen_fd; // 가장 큰 파일 디스크립터 값 설정
	// 무한루프: 서버가 지속적으로 클라이언트 요청을 처리하고 연결을 유지
	while(1)
	{
		allfds = readfds; // // 모든 소켓 복사
		printf("Select Wait %d\n", maxfd);
		// 소켓 이벤트 대기
		fd_num = select(maxfd + 1 , &allfds, (fd_set *)0,
					  (fd_set *)0, NULL);

		// 리슨 소켓에 이벤트 발생 (새로운 클라이언트 연결 요청)
		if (FD_ISSET(listen_fd, &allfds))
		{
			addrlen = sizeof(client_addr); // 클라이언트 주소 크기 설정
			client_fd = accept(listen_fd,
					(struct sockaddr *)&client_addr, &addrlen); // 연결 수락

			FD_SET(client_fd,&readfds); // 새로운 클라이언트 소켓 추가

			if (client_fd > maxfd)
				maxfd = client_fd; // 가장 큰 파일 디스크립터 업데이트
			printf("Accept OK\n");
			continue;
		}
		// 기존 소켓에서 데이터 수신 및 처리
		for (i = 0; i <= maxfd; i++)
		{
			sockfd = i; // sockfd : 현재 처리중인 서버 소켓 파일 디스크립터
			if (FD_ISSET(sockfd, &allfds))
			{
				memset(buf, 0x00, MAXLINE); // 버퍼 초기화
				// read : sockfd 소켓에서 MAXLINE 바이트만큼 데이터를 읽어 buf(버퍼)에 저장
				// read(int fd, void *buf, size_t count); 의 반환값이 0 : 서버가 연결을 종료함
				if (read(sockfd, buf, MAXLINE) <= 0)
				{
					close(sockfd);
					FD_CLR(sockfd, &readfds);
				}
				else
				{
					if (strncmp(buf, "quit\n", 5) ==0)
					{
						close(sockfd);
						FD_CLR(sockfd, &readfds);
					}
					else
					{
						printf("Read : %s", buf);
						write(sockfd, buf, strlen(buf)); // 에코 서버 : 받은 데이터를 그대로 전송
					}
				}
				if (--fd_num <= 0)
					break;
			}
		}
	}
}

