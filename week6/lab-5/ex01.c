#include <stdio.h>
int main() {
    int i, array[10];
    for(i=0; i<10; i++){
        printf("Enter the value %d here: ", i+1);
        scanf("%d", &array[i]);
    }
    printf("Values in array are: ");
    for(i=0; i<9; i++){
        printf("%d, ", array[i]);
    }
    printf("%d\n", array[9]);
    return 0;
}