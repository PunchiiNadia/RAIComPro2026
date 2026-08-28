#include <stdio.h>
#define NUM_STUDENT 3
#define NUM_SUBJ 4

int main()
{
    struct Student {
        char name[20];
        int id, grade[NUM_SUBJ];
        float average;
    } s[NUM_STUDENT];

    char subject[NUM_SUBJ][20] = {"Math", "English", "Science", "History"};
    float total[NUM_SUBJ] = {0}, avg[NUM_SUBJ];
    int i, j, topStudent = 0, topSubject = 0;

    for (i = 0; i < NUM_STUDENT; i++) {
        printf("Enter name, ID, and 4 grades for student %d:\n", i + 1);
        scanf("%20s %d\n", s[i].name, &s[i].id);
        printf("\n");
        s[i].average = 0;
        for (j = 0; j < NUM_SUBJ; j++) {
            scanf("%d", &s[i].grade[j]);
            s[i].average += s[i].grade[j];
            total[j] += s[i].grade[j];
        }
        s[i].average /= NUM_SUBJ;
        if (i > 0 && s[i].average > s[topStudent].average) topStudent = i;
    }

    printf("Student Averages:\n");
    for (i = 0; i < NUM_STUDENT; i++)
        printf("%s (ID: %d): %.2f\n", s[i].name, s[i].id, s[i].average);
    
    printf("\n");
    printf("Top Student: %s with %.2f\n\n", s[topStudent].name, s[topStudent].average);
    printf("Subject Averages:\n");

    for (j = 0; j < NUM_SUBJ; j++) {
        avg[j] = total[j] / NUM_STUDENT;
        printf("%s: %.2f\n", subject[j], avg[j]);
        if (j > 0 && avg[j] > avg[topSubject]) topSubject = j;
    }

    printf("\n");
    printf("Top Subject: %s with average %.2f\n", subject[topSubject], avg[topSubject]);
    return 0;
}