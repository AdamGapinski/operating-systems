#ifndef ADDRESSBOOK_STRUCTURES_H
#define ADDRESSBOOK_STRUCTURES_H

typedef struct date {
    short day;
    short month;
    short year;
} date;

typedef struct addressStr {
    char *country;
    char *city;
    char *streetAddress;
    char *postalCode;
} addressStr;

typedef struct contactStr {
    char *name;
    char *surname;
    char *email;
    char *phone;
    date *dateOfBirth;
    addressStr *address;
} contactStr;

typedef struct listNode {
    contactStr *contact;
    struct listNode *next;
    struct listNode *previous;
} listNode;

typedef struct linkedBook {
    listNode *head;
} linkedBook;

typedef struct treeNode {
    contactStr *contact;
    struct treeNode *right;
    struct treeNode *left;
} treeNode;

typedef struct bTBook {
    treeNode *root;
} bTBook;


#endif //ADDRESSBOOK_STRUCTURES_H
