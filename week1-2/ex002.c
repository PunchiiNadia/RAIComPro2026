#include <stdio.h>
int main() {
    int a, b, c;
    printf("Enter the first value:");
    scanf("%d", &a);
    printf("Enter the secind value:");
    scanf("%d", &b);
    c = a + b;
    printf("%d + %d = %d\n", a, b, c);
    
    return 0;
}