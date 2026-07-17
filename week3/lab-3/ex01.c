#include <stdio.h>
int main() {
    int int_val;
    float float_val;
    char char_val[1];

    printf("Please enter an integer value: ");
    scanf("%d", &int_val);
    printf("You entered %d\n", int_val);

    printf("Please enter a float value: ");
    scanf("%f", &float_val);
    printf("You entered %0.1f\n", float_val);

    printf("Please enter a character: ");
    scanf("%s", char_val);
    printf("You entered %s\n", char_val);
    return 0;
}