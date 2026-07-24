#include <stdio.h>
int main() {
    char name[99], surname[99], student_id[99];
    int lab, mid, final, total;

    printf("Enter your Name Surname : ");
    scanf(" %s %s", name, surname);
    printf("Enter your Student ID : ");
    scanf("%s", student_id);
    printf("Enter your score\n");
    printf("\tLaboratory (15%) %s", "= ");
    scanf("%d", &lab);
    printf("\tMidterm (35%) %5s", "= ");
    scanf("%d", &mid);
    printf("\tfinal (50%) %7s", "= ");
    scanf("%d", &final);

    total = lab+mid+final;
    printf("Total (100%) %13s %d", "=", total);
    return 0;
}