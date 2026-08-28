#include <stdio.h>

struct Book {
    char name[100];
    float price;
    float discount;
    float totalPrice;
};

int main() {
    struct Book book;

    printf("Enter book name : ");
    scanf(" %[^\n]", book.name);

    printf("Enter book price : ");
    scanf("%f", &book.price);

    book.discount = book.price * 0.10;
    book.totalPrice = book.price - book.discount;

    printf("\nBook : %s\n", book.name);
    printf("Price : %.2f\n", book.price);
    printf("Discount 10 percent : %.2f\n", book.discount);
    printf("Total price : %.2f\n", book.totalPrice);

    return 0;
}