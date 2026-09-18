#include <stdio.h>

int sum_square_odd (int start, int end){
    int sum = 0;
    for (int i=start; i<end; i++) {
        if(i%2==1) {
            sum+=i*i;
        }
    }
    return sum;
}

int main() {
    int start, end;
    printf("Enter the two integers (start end): ");
    scanf("%d %d", &start, &end);
    printf("Sum of squares of odd numbers from %d to %d is %d\n", start, end, sum_square_odd(start, end));
}