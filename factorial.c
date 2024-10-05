#include <stdio.h>

int factorial(int n){
    int fac = 1;
    for(int i = 1; i <= n; i++){
        fac *= i;
    }
    return fac;
}