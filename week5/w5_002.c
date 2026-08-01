#include <stdio.h>
int main() {
    int num =1, sum_val=0;
    do {
        sum_val = sum_val + num;
        num++;
    }
    while (num<=30);
    printf("Summation of 1 to 30 = %d\n", sum_val);
    return 0;
}