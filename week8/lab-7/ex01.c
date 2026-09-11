#include <stdio.h>

int main() {
    int test = 0;
    int *p;

    p = &test;

    printf("The address of test variable is at: %p\n", (void *)p);

    *p = 99;

    printf("The new value of test via pointer is: %d\n", *p);

    return 0;
}