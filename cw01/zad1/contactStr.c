#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "include/contactStr.h"

date *createDate(short day, short month, short year) ;

addressStr *createAddress(char *country, char *city, char *streetAddress, char *postalCode) ;

contactStr *createContact(char *name, char *surname, char *email, char *phone, short day, short month, short year,
                          char *country, char *city, char *streetAddress, char *postalCode) {
    contactStr *contact = malloc(sizeof(*contact));
    contact->name = strdup(name);
    contact->surname = strdup(surname);
    contact->email = strdup(email);
    contact->phone = strdup(phone);
    contact->dateOfBirth = createDate(day, month, year);
    contact->address = createAddress(country, city, streetAddress, postalCode);

    return contact;
}

contactStr *duplicateContact(contactStr *to_duplicate) {
    contactStr *result = malloc(sizeof(*result));
    result->name = strdup(to_duplicate->name);
    result->surname = strdup(to_duplicate->surname);
    result->email = strdup(to_duplicate->email);
    result->phone = strdup(to_duplicate->phone);
    result->dateOfBirth = createDate(to_duplicate->dateOfBirth->day,
                                     to_duplicate->dateOfBirth->month,
                                     to_duplicate->dateOfBirth->year);
    result->address = createAddress(to_duplicate->address->country,
                                    to_duplicate->address->city,
                                    to_duplicate->address->streetAddress,
                                    to_duplicate->address->postalCode);

    return result;
}

void printContact(contactStr *contact) {
    if (contact == NULL) return;
    printf("[Name: %s, surname: %s, email: %s, phone: %s, date of birth: %d.%d.%d\nAddress: %s %s %s %s]\n",
           contact->name, contact->surname, contact->email, contact->phone,
           contact->dateOfBirth->day, contact->dateOfBirth->month, contact->dateOfBirth->year,
           contact->address->streetAddress, contact->address->postalCode, contact->address->city,
           contact->address->country);
}

date *createDate(short day, short month, short year) {
    date *date = malloc(sizeof(*date));
    date->day = day;
    date->month = month;
    date->year = year;
    return date;
}

addressStr *createAddress(char *country, char *city, char *streetAddress, char *postalCode) {
    addressStr *address = malloc(sizeof(*address));
    address->country = strdup(country);
    address->city = strdup(city);
    address->streetAddress = strdup(streetAddress);
    address->postalCode = strdup(postalCode);

    return address;
}

void deleteContact(contactStr **pointer) {
    contactStr *pContact = *pointer;

    free(pContact->name);
    free(pContact->surname);
    free(pContact->email);
    free(pContact->phone);
    free(pContact->dateOfBirth);
    free(pContact->address->country);
    free(pContact->address->city);
    free(pContact->address->streetAddress);
    free(pContact->address->postalCode);

    *pointer = NULL;
    free(pContact);
}

int compareBySurname(contactStr *first, contactStr *second) {
    return strcmp(first->surname, second->surname);
}

int compareByName(contactStr *first, contactStr *second) {
    return strcmp(first->name, second->name);
}

int compareByDateOfBirth(contactStr *first, contactStr *second) {
    if (first->dateOfBirth->year != second->dateOfBirth->year) {
        return second->dateOfBirth->year - first->dateOfBirth->year;
    } else if (first->dateOfBirth->month != second->dateOfBirth->month) {
        return second->dateOfBirth->month - first->dateOfBirth->month;
    } else {
        return second->dateOfBirth->day - first->dateOfBirth->day;
    }
}

int compareByEmail(contactStr *first, contactStr *second) {
    return strcasecmp(first->email, second->email);
}

int compareByPhone(contactStr *first, contactStr *second) {
    return strcmp(first->phone, second->phone);
}
