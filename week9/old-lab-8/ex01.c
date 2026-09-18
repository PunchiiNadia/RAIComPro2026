#include <stdio.h>
#define PI 22.0/7.0

float circum(float r) {
    float answer = 0;
    answer = 2 * PI * r;
    return answer;
}

float area(float r) {
    float answer = 0;
    answer = PI * r * r;
    return answer;
}

int main() {
    float r;
    printf("Input radius: ");
    scanf("%f", &r);
    printf("Circumference: %.2f\n", circum(r));
    printf("Area: %.2f\n", area(r));
    return 0;
}