#include <stdio.h>

int main() {
    int array[] = {3, 1, 2, 4, 5, 6};
    int *p = array;

    int max = *p;
    int min = *p;

    for (int i = 1; i < 6; i++) {
        if (*(p + i) > max) {
            max = *(p + i);
        }

        if (*(p + i) < min) {
            min = *(p + i);
        }
    }

    printf("Max value: %d\n", max);
    printf("Min value: %d\n", min);

    return 0;
}