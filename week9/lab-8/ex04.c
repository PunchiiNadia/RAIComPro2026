#include <stdio.h>
#include <ctype.h>

void count_character(const char *str) {
    int letters = 0;
    int digits = 0;
    int spe_char = 0;

    for(int i=0; str[i] != '\0'; i++) {
        if(isalpha(str[i])) {
            letters++;
        } 
        else if(isdigit(str[i])) {
            digits++;
        } 
        else {
            spe_char++;
        }
    }
    printf("Output:\n");
    printf("Letters: %d\n", letters);
    printf("Digits: %d\n", digits);
    printf("Special Characters / Symbols: %d\n", spe_char);
}

int main() {
    char str[100];
    printf("Input: ");
    scanf("%[^\n]", str);
    count_character(str);
}