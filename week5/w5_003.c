#include <stdio.h>
int main() {
    int sum_val=0, num;
    for(num=1; num<=30; num++) {
        sum_val = sum_val + num;
    }printf("Summation of 1 to 30 = %d\n", sum_val);
    return 0;
}