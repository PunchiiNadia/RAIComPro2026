#include <stdio.h>
int main() {
    float height, base_radius, volume;
    printf("Enter cone height: ");
    scanf("%f", &height);
    printf("Enter base radius: ");
    scanf("%f", &base_radius);
    volume = 0.333333*3.14159*(base_radius*base_radius)*height;
    printf("Cone volume = %.1f\n", volume);
    if (volume > 260) {
        printf("This cone is perfect for Supun project\n");
    }
    else {
        printf("This cone is not fit for this project\n");
    }
}