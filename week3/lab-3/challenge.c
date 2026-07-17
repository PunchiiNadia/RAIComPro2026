#include <stdio.h>
int main() {
    int val_1,val_2,val_3,sum;
    float avg;

    printf("Enter integer #1: ");
    scanf("%d", &val_1);

    printf("Enter integer #2: ");
    scanf("%d", &val_2);

    printf("Enter integer #3: ");
    scanf("%d", &val_3);

    int max = val_1;
    if (val_2 > max) max = val_2;
    if (val_3 > max) max = val_3;

    int min = val_1;
    if (val_2 < min) min = val_2;
    if (val_3 < min) min = val_3;



    sum = val_1+val_2+val_3;
    avg = sum/3.0;

    printf("Results:\n");
    printf("Minimum: %d\n", min);
    printf("Maximum: %d\n", max);
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", avg);
}