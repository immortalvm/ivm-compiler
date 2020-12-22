#include <stdio.h>
#include <stdlib.h>

void goodbye(){
    printf("Bye\n");
}

int main(){
    atexit(goodbye);
    printf("Hello\n");
    return 126;
}

