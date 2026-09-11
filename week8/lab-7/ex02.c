#include <stdio.h>

int main() {
    int a = 10;
    int b = 20;
    int c = 30;

    int *p1 = &a;
    int *p2 = &b;
    int *p3 = &c;

    int temp;

    printf("Before swap: a=%d, b=%d, c=%d\n", a, b, c);

    temp = *p1;
    *p1 = *p2;
    *p2 = *p3;
    *p3 = temp;

    printf("After swap: a=%d, b=%d, c=%d\n", a, b, c);

    return 0;
}