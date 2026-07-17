#include <stdio.h>
int main() {
    int total_time,hour,min;

    printf("Enter total minutes: ");
    scanf("%d", &total_time);

    hour=total_time/60;
    min=total_time%60;

    printf("%d minutes is %d hour(s) and %d minute(s)", total_time, hour, min);

    return 0;
}