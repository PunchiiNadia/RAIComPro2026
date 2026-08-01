#include <stdio.h>
int main()
{
    float total_sum=0, num, avg;
    for(int count=1; count<=10; count++){
        printf("%d.Enter the number: ", count);
        scanf("%f", &num);
        total_sum = num+total_sum;
    }
    printf("total sum is %.0f\n", total_sum);
    avg = total_sum/10;
    printf("Average is %.2f\n", avg);
}