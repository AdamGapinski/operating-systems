#ifndef ADDRESSBOOK_LINKEDLISTBOOK_H
#define ADDRESSBOOK_LINKEDLISTBOOK_H

#include "structures.h"
#include "contactStr.h"

linkedBook *createLinkedBook();
void deleteLinkedBook(linkedBook **pBook);
void addContactToLinkedBook(linkedBook *book, contactStr *contact);
void deleteContactLinkedBook(linkedBook *book, contactStr **pContact);
contactStr *findContactByPhoneLBook(linkedBook *book, char *phone);
contactStr *findContactByEmailLBook(linkedBook *book, char *email);
void sortLinkedBookBySurname(linkedBook *book);
void sortLinkedBookByName(linkedBook *book);
void sortLinkedBookByDateOfBirth(linkedBook *book);
void sortLinkedBookByEmail(linkedBook *book);
void sortLinkedBookByPhone(linkedBook *book);
void printLinkedBook(linkedBook *book);

#endif
