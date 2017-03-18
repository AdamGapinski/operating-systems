#ifndef ADDRESSBOOK_CONTACT_H
#define ADDRESSBOOK_CONTACT_H

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

contactStr *createContact(char *name, char *surname, char *email, char *phone, short day, short month, short year,
                              char *country, char *city, char *streetAddress, char *postalCode);
contactStr *duplicateContact(contactStr *to_duplicate);
void deleteContact(contactStr **contact);
void printContact(contactStr *contact);

typedef int (*compareContacts)(contactStr*, contactStr*);
int compareBySurname(contactStr *first, contactStr *second);
int compareByName(contactStr *first, contactStr *second);
int compareByDateOfBirth(contactStr *first, contactStr *second);
int compareByEmail(contactStr *first, contactStr *second);
int compareByPhone(contactStr *first, contactStr *second);

#endif
