#include <stdio.h>
#define SIZE 8
int main() {
    int i, array[SIZE], min, max, temp;
    for(i=0; i<SIZE; i++){
        printf("Enter number %d: ", i+1);
        scanf("%d", &array[i]);
    }
    min=array[0];
    max=array[0];
    for(i=0; i<SIZE; i++){
        if(array[i]<min){
            min=array[i];
        }
        if(array[i]>max){
            max=array[i];
        }
    }
    printf("Smallest number: %d \n", min);
    printf("Largest number: %d \n", max);

    return 0;
}