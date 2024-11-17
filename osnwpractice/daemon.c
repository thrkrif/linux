#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

// 데몬 프로세스 예제
int main(){
    int pid;
    int i = 0;
    printf("process start %d\n", getpid()); // getpid()는 위의 int pid를 나타내는 건가?
    pid = fork(); // 자식 프로세스 생성
    
    if(pid > 0){
        printf("parent process id(%d)\n", getpid()); // 부모 프로세스인데 왜 getppid()가 아님?
        exit(0);
    }
    else if(pid == 0){
        printf("child process id(%d) : ppid(%d)\n",getpid(), getppid());
        close(0);
        close(1);
        close(2); // 표준 입출력, 에러 닫기
        setsid(); // 세션 생성
        printf("i'm daemon\n");
        i = 1000;
        while(1){   // 데몬 프로그램이 실행할 코드
            printf("child : %d\n",i);
            i++;
            sleep(2);
        }
    }
    return 1; // 왜 return 0이 아니라 return 1?
}