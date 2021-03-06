#ifndef ADDRESSBOOK_BINARYTREEBOOK_H
#define ADDRESSBOOK_BINARYTREEBOOK_H

#include "structures.h"
#include "contactStr.h"

bTBook *createBTBook();
void deleteBTBook(bTBook **pBook);
void addContactToBTBook(bTBook *book, contactStr *contact);
void deleteContactBTBook(bTBook *book, contactStr **pContact);
contactStr *findContactByPhoneBTBook(bTBook *book, char *phone);
contactStr *findContactByEmailBTBook(bTBook *book, char *email);
void sortBTBookBySurname(bTBook *book);
void sortBTBookByName(bTBook *book);
void sortBTBookByDateOfBirth(bTBook *book);
void sortBTBookByEmail(bTBook *book);
void sortBTBookByPhone(bTBook *book);
void printBTBook(bTBook *book);

#endif
