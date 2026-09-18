#include <stdio.h>

int sum_square (int a, int b) {
    int sq_a = a*a;
    int sq_b = b*b;

    int answer = sq_a+sq_b;
    return answer;
}

int main() {
    int a;
    int b;
    int answer;
    printf("Enter the two integers: ");
    scanf("%d %d", &a, &b);
    answer = sum_square(a, b);
    printf("Sum of squares of %d and %d is %d\n", a, b, answer);
}