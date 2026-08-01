#include <stdio.h>
int main(){
    int num,count=1,total_sum=0;

while(count <= 10){
        printf("%d.Enter the number: ", count);
        scanf("%d", &num);
        total_sum = num+total_sum;
        count++;
}

printf("Total sum is %d\n", total_sum);
}