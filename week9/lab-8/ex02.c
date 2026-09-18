#include <stdio.h>

int is_palindrome(int num) {
    int orginalNum=num, reversedNum=0, remainder;
    while(num!=0) {
        remainder = num%10;
        reversedNum = reversedNum * 10 + remainder;
        num /=10;
    }
    return (orginalNum == reversedNum);
}
int main() {
    int start, end, count=0, total_sum=0;
    printf("Enter the start and end numbers: ");
    scanf("%d %d", &start, &end);
    printf("The palindrome numbers within the interval [%d, %d] are: \n", start, end);
    for(int i=start; i<=end; i++){
        if (is_palindrome(i)) {
            printf("%d ", i);
        }
    }
    for(int i=start; i<=end; i++) {
        if(is_palindrome(i)) {
            count++;
            total_sum+=i;
        }
    }
    printf("Total palindrome numbers: %d\n", count);
    printf("Sum of palindrome numbers: %d\n", total_sum);
}
