#include <stdio.h>
// 함수 선언
void hello();
void world();

void main(){
    printf("OSNW2024\t");
    // 함수 호출
    hello();
    world();    
    
}

// 함수 정의
void hello(){
 printf("Hello\t");
}

void world(){
printf("World!\n");
}