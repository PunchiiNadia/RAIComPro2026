#include <stdio.h>
#include<math.h>

float area(float a, int b, int c) {
    float area;
    float s;
    s = (a + b + c)/2;
    area = sqrt(s*(s-a)*(s-b)*(s-c));
    return area;
}

int main() {
    float a, b, c;
    printf("Input 3 Sides of the triangle\n");
    printf("Inut Side 1: ");
    scanf("%f", &a);
    printf("Inut Side 2: ");
    scanf("%f", &b);
    printf("Inut Side 3: ");
    scanf("%f", &c);
    printf("The area of the triangle is: %.2f\n", area(a, b, c));
}