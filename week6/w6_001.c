#include <stdio.h>
int main() {
    int row, column, matrix[3][3];
    for(row=0; row<3; row++){
        for(column=0; column<3; column++){
            printf("Enter number[%d][%d]: ", row, column);
            scanf("%d", &matrix[row][column]);
        }
    }
    printf("\n");
    printf("==Matrix[3]x[3]==\n");
    for(row=0; row<3; row++){
        for(column=0; column<3; column++){
            printf("%d\t", matrix[row][column]);
        }
        printf("\n");
    }
    printf("\n");
    return 0;
}