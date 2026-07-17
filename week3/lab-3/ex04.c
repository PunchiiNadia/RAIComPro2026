#include <stdio.h>
int main() {
    char name[99];
    int student_id,prog_score;
    float phy_score,cal_score;

    printf("Enter your Name: ");
    scanf("%s", name);

    printf("Enter your student ID: ");
    scanf("%d", &student_id);

    printf("Enter your Programming score: ");
    scanf("%d", &prog_score);

    printf("Enter your Physics score: ");
    scanf("%f", &phy_score);

    printf("Enter your Calculus score: ");
    scanf("%f", &cal_score);

    float gpa_score = (prog_score + phy_score + cal_score)/3;
    printf("Hi %s(%d)! Your GPA is %.2f\n", name, student_id, gpa_score);
}