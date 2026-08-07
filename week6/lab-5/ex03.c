#include <stdio.h>
#define SIZE 5
int main() {
    int i, student_score[SIZE], total_mark, max;
    for(i=0; i<SIZE; i++){
        printf("Enter the marks of student %d: ", i+1);
        scanf("%d", &student_score[i]);
    }
    for(i=0; i<SIZE; i++){
        total_mark=total_mark+student_score[i];
    }
    printf("Total Marks : %d\n", total_mark);
    max=student_score[0];
    for(i=0; i<SIZE; i++){
        if(student_score[i]>max){
            max=student_score[i];
        }
    }
    printf("Highest Marks: %d\n", max);
    return 0;
}
    