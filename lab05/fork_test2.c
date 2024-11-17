#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv){
    pid_t pid1, pid2;
    int i = 100;

    printf("Program Start!!\n");
    printf("hit a key "); getchar(); printf("\n");

    pid1 = fork();  // 프로세스 복사 
    if(pid1 < 0){
        printf("fork failure\n");
        return 1;
    }

    pid2 = fork();  // 프로세스 복사 / 부모 프로세스의 자식, pid1의 자식이 만들어짐
    if(pid2 < 0){
        printf("fork failure\n");
        return 1;
    }

    printf("Process %d created by parent %d\n", getpid(), getppid());

    while(1) {  // 각 프로세스가 무한 루프를 실행
        printf("Process ID: %d, i = %d\n", getpid(), i);
        i++;
        sleep(1);
    }

    return 0;
}