#include <stdio.h>

struct Address
{
    int houseNo;
    int moo;
    char road[30];
    char district[30];
    char province[30];
};

struct Student
{
    char fullName[50];
    int id;
    struct Address address;
    char tel[20];
};

int main()
{
    struct Student student1 = {
        "POOM KONGHUAYROB",
        69011014,
        {3, 2, "Chalongkrung Rd.", "Ladkrabang", "Bangkok"},
        "088 888 8888"
    };

    printf("Full name : %s, ID : %d\n",
           student1.fullName, student1.id);

    printf("Address : %d Moo %d, %s\n",
           student1.address.houseNo,
           student1.address.moo,
           student1.address.road);

    printf("%s, %s\n",
           student1.address.district,
           student1.address.province);

    printf("\nTel. %s\n", student1.tel);

    return 0;
}