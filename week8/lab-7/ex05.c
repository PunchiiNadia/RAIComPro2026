#include <stdio.h>
int main() {
    int size = 6;
    int array[] = {3, 1, 2, 4, 5, 6};
    int *p = array;
    int sum = 0;

    for(int i=0; i<size; i++){
        if (*(p+i)%2==0) {
            sum += *(p+i);
        }
    }

    printf("The sum of even numbers is: %d\n", sum);
    return 0;
}