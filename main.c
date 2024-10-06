#include <stdio.h>
// 파일 3개 만들거임, main, hello, world
// 함수 선언
// extern이란 이 파일에서 정의되지 않았음을 의미한다.
extern void id();
extern int factorial(int num);

int main(){
    printf("OSNW2024\t");
    // 함수 호출
    id();
    printf("fact10 = %d\n",factorial(10));    
}