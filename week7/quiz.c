#include <stdio.h>

#define ROW 3
#define COL 3

int main() {
    int A[ROW][COL] = {
        {1, 0, -1},
        {-1, 2, 3},
        {2, 4, 5}
    };

    int i, j;

    // Convert A into a diagonal matrix
    for (i = 0; i < ROW; i++) {
        for (j = 0; j < COL; j++) {
            if (i != j) {
                A[i][j] = 0;
            }
        }
    }

    // Display the result
    printf("Diagonal matrix of A:\n");

    for (i = 0; i < ROW; i++) {
        for (j = 0; j < COL; j++) {
            printf("%d\t", A[i][j]);
        }
        printf("\n");
    }

    return 0;
}