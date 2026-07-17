#include <stdio.h>
int main() {
    char first_name[99],last_name[99],uni_name[99];
    int age;
    float height;

    printf("Enter your full name: ");
    scanf("%s %s", first_name, last_name);

    printf("Enter your age: ");
    scanf(" %d", &age);

    printf("Enter your height: ");
    scanf(" %f", &height);  

    printf("Enter your University name: ");
    scanf(" %s", uni_name);

    printf("Hi! Everyone. This is %c.%s from %s. I am %d years old and my height is %0.1f cm tall.\n", last_name[0], first_name, uni_name, age, height);
}