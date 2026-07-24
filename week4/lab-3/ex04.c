#include <stdio.h>
int main() {
    char name[99];
    char grade;
    float cal, physics, sci, avg_score;
    printf("Enter your name: ");
    scanf("%s", name);
    printf("Enter your Calculus score: ");
    scanf("%f", &cal);
    printf("Enter your Physics score: ");
    scanf("%f", &physics);
    printf("Enter your Science score: ");
    scanf("%f", &sci);
    avg_score = (cal+physics+sci)/3;
    if (avg_score >= 80) {
        grade = 'A';
    }
    else if (avg_score >= 70) {
        grade = 'B';
    }
    else if (avg_score >= 60) {
        grade = 'C';
    }
    else if (avg_score >= 50) {
        grade = 'D';
    }
    else {
        grade = 'F';
    }
    printf("%s, your average is %0.2f. You got grade %c.\n", name, avg_score, grade);
}