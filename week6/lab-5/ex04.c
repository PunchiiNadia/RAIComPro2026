#include <stdio.h>
#define SIZE 10
int main() {
    int i, array[SIZE], even_count=0, odd_count=0;
    for(i=0; i<10; i++){
        printf("Enter value %d: ", i+1);
        scanf("%d", &array[i]);
    }
    for(i=0; i<10; i++){
        if(array[i]%2==0){
            even_count++;
        }
        else {
            odd_count++;
        }
    }
    printf("Even numbers: %d\n", even_count);
    printf("Odd numbers: %d\n", odd_count);
    return 0;
}
    