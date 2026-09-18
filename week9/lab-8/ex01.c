#include <stdio.h>

float area(float length, float width) {
    float area = length * width;
    return area;
}

float base_p(float length, float width) {
    float base_p = 2*(length+width);
    return base_p;
}

float volume(float length, float width, float height) {
    float v = length*width*height;
    return v;
}

int main() {
    float length, width, height;
    while (1) {
        printf("Enter length, width, and height in cm: ");
        scanf("%f %f %f", &length, &width, &height);

        if (length >=0 && width >=0 && height >=0) {
            printf("Base Area: %.2f\n", area(length, width));
            printf("Perimeter: %.2f\n", base_p(length, width));
            printf("Volume: %.2f\n", volume(length, width, height));
            break;
        } 
        else {
            printf("Error: All dimensions must be greater than 0.\n");
        }
    }
}
