#include <stdio.h>

float cal_avg(int grades[], int n){
    float sum=0.0;
    for(int i=0; i<n; i++){
        sum+=grades[i];
    }
    return sum/n;
}

int find_max(int grades[], int n){
    int max = grades[0];
    for(int i=1; i<n; i++) {
        if(grades[i] > max) {
            max=grades[i];
        }
    }
    return max;
}

int find_min(int grades[], int n){
    int min = grades[0];
    for(int i=1; i<n; i++) {
        if(grades[i] < min) {
            min=grades[i];
        }
    }
    return min;
}

void print_passed(char names[][50], int grades[], int n){
    for(int i=0; i<n; i++) {
        if(grades[i]>=80) {
            printf("%s - Grade A\n", names[i]);
        }
        else if(grades[i]>=70) {
            printf("%s - Grade B\n", names[i]);
        }
        else if(grades[i]>=60) {
            printf("%s - Grade C\n", names[i]);
        }
    }
}
int main() {
    int n;
    char names[100][50];
    int grades[100];

    printf("Enter number of students: ");
    scanf("%d", &n);
    
    for(int i=0; i<n; i++){
        printf("Enter name of student %d: ", i+1);
        scanf("%s", names[i]);
        printf("Enter grade of %s: ",names[i]);
        scanf("%d", &grades[i]);
    }
    printf("--- Results ---\n");
    printf("Average grade: %.2f\n", cal_avg(grades, n));
    printf("Highest grade: %d\n", find_max(grades, n));
    printf("Lowest grade: %d\n", find_min(grades, n));

    printf("Students who passed: \n");
    print_passed(names, grades, n);

}