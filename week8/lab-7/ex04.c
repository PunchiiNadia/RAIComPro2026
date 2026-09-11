#include <stdio.h>

int main() {
    int size = 6;
    int array[] = {3, 1, 2, 4, 5, 6};

    int *p = array + size - 1;

    for (int i = 0; i < size; i++) {
        printf("%d\n", *p);
        p--;
    }

    return 0;
}