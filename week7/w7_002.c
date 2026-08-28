#include <stdio.h>
#include <string.h>

int main()
{
    int i;

    struct profile
    {
        char name[20];
        int age;
    } s[10];

    for (i = 0; i < 10; i++)
    {
        printf("Student[%d]\n", i + 1);

        printf("\tName: ");
        scanf("%s", s[i].name);

        printf("\tAge: ");
        scanf("%d", &s[i].age);
    }

    printf("\nStudents older than 20 years old:\n");

    for (i = 0; i < 10; i++)
    {
        if (s[i].age > 20)
        {
            printf("%s, %d years old\n", s[i].name, s[i].age);
        }
    }

    return 0;
}