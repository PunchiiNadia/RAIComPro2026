#include <stdio.h>

int sum_even(int arr[], int n) {
    int sum=0;
    for(int i=0; i<n; i++) {
        if(i%2==0) {
            sum += arr[i];
        }
    }
    return sum;
}

int sum_odd(int arr[], int n) {
    int sum=0;
    for(int i=0; i<n; i++) {
        if(i%2==1) {
            sum += arr[i];
        }
    }
    return sum;
}

float avg_even(int arr[], int n) {
    int count = (n + 1) / 2;
    return (float)sum_even(arr, n)/count;
}

float avg_odd(int arr[], int n) {
    int count = n / 2;
    return (float)sum_odd(arr, n)/count;
}

int main() {
    int n;
    printf("N: ");
    scanf("%d", &n);

    int arr[n];
    for(int i=0; i<n; i++) {
        printf("Input: ");
        scanf("%d", &arr[i]);
    }
    printf("Output:\n");
    printf("Even index elements: - > Sum: %d, Average : %.2f\n", sum_even(arr, n), avg_even(arr, n));
    printf("Odd index elements: - > Sum: %d, Average : %.2f\n", sum_odd(arr, n), avg_odd(arr, n));
}