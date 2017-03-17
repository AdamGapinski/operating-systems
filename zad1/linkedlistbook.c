#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "include/linkedlistbook.h"

void insertNode(linkedBook *book, listNode *node) ;

listNode *getNode(const linkedBook *book, const contactStr *contact) ;

void removeNode(linkedBook *book, listNode *node) ;

linkedBook *sort(linkedBook *book, compareContacts compareFunction) ;

linkedBook *concatBooks(linkedBook *left, listNode *pivot, linkedBook *right);

linkedBook *createLinkedBook() {
    linkedBook *book = malloc(sizeof(*book));
    book->head = NULL;
    return book;
}

void deleteLinkedBook(linkedBook **pointer) {
    linkedBook *pBook = *pointer;

    while(pBook->head != NULL) {
        deleteContact(&pBook->head->contact);
        removeNode(pBook, pBook->head);
    }

    *pointer = NULL;
    free(pBook);
}

void addContactToLinkedBook(linkedBook *book, contactStr *contact) {
    listNode *node = malloc(sizeof(*node));
    node->contact = contact;
    insertNode(book, node);
}

void deleteContactLinkedBook(linkedBook *book, contactStr **contact) {
    removeNode(book, getNode(book,*contact));
    deleteContact(contact);
}

contactStr* findContactByPhoneLBook(linkedBook *book, char *phone) {
    if (book->head == NULL) return NULL;
    listNode *current = book->head;

    do {
        if (strcmp(current->contact->phone, phone) == 0) {
            return current->contact;
        }
        current = current->next;
    } while (current != NULL);

    return NULL;
}

contactStr* findContactByEmailLBook(linkedBook *book, char *email) {
    if (book->head == NULL) return NULL;
    listNode *current = book->head;

    do {
        if (strcmp(current->contact->email, email) == 0) {
            return current->contact;
        }
        current = current->next;
    } while (current != NULL);

    return NULL;
}

void sortLinkedBookBySurname(linkedBook *book) {
    linkedBook *sorted = sort(book, &compareBySurname);
    book->head = sorted->head;
    if (sorted != book) {
        free(sorted);
    }
}

void sortLinkedBookByName(linkedBook *book) {
    linkedBook *sorted = sort(book, &compareByName);
    book->head = sorted->head;
    if (sorted != book) {
        free(sorted);
    }
}

void sortLinkedBookByDateOfBirth(linkedBook *book) {
    linkedBook *sorted = sort(book, &compareByDateOfBirth);
    book->head = sorted->head;
    if (sorted != book) {
        free(sorted);
    }
}

void sortLinkedBookByEmail(linkedBook *book) {
    linkedBook *sorted = sort(book, &compareByEmail);
    book->head = sorted->head;
    if (sorted != book) {
        free(sorted);
    }
}

void sortLinkedBookByPhone(linkedBook *book) {
    linkedBook *sorted = sort(book, &compareByPhone);
    book->head = sorted->head;
    if (sorted != book) {
        free(sorted);
    }
}

void insertNode(linkedBook *book, listNode *node) {
    node->next = NULL;
    node->previous = NULL;
    if (book->head == NULL) {
        book->head = node;
    } else {
        book->head->previous = node;
        node->next = book->head;
        book->head = node;
    }
}

void printLinkedBook(linkedBook *book) {
    listNode *current = book->head;

    while(current != NULL) {
        printContact(current->contact);
        current = current->next;
    }
}

void removeNode(linkedBook *book, listNode *node) {
    if (node != NULL) {
        if (node->previous != NULL) {
            node->previous->next = node->next;
        } else {
            book->head = node->next;
        }
        if (node->next != NULL) {
            node->next->previous = node->previous;
        }

        free(node);
    }
}

listNode *getNode(const linkedBook *book, const contactStr *contact) {
    listNode *current = book->head;
    listNode *found = NULL;

    while(found == NULL && current != NULL) {
        if (current->contact == contact) {
            found = current;
        }

        current = current->next;
    }
    return found;
}

linkedBook *sort(linkedBook *book, compareContacts compareFunction) {
    if (book->head == NULL) return book;

    linkedBook *lesser = calloc(1, sizeof(*lesser));
    linkedBook *greaterOrEq = calloc(1, sizeof(*greaterOrEq));

    listNode *pivot = book->head;
    listNode *current = book->head->next;

    while(current != NULL) {
        listNode *node = current;
        current = current->next;

        if (compareFunction(node->contact, pivot->contact) >= 0) {
            insertNode(greaterOrEq, node);
        } else {
            insertNode(lesser, node);
        }
    }

    linkedBook *result = concatBooks(sort(lesser, compareFunction), pivot, sort(greaterOrEq, compareFunction));
    if (result != lesser) {
        free(lesser);
    }
    if (result != greaterOrEq) {
        free(greaterOrEq);
    }

    return result;
}

listNode *getLast(linkedBook *book) {
    if (book == NULL || book->head == NULL) return NULL;
    listNode *current = book->head;
    listNode *next = book->head->next;
    while(next != NULL) {
        current = next;
        next = next->next;
    }
    return current;
}

linkedBook *concatBooks(linkedBook *left, listNode *pivot, linkedBook *right) {
    if (left->head == NULL) {
        insertNode(right, pivot);
        return right;
    }

    if (right->head == NULL) {
        listNode *last = getLast(left);
        pivot->previous = last;
        pivot->next = NULL;
        last->next = pivot;

        return left;
    };

    insertNode(right, pivot);

    listNode *lastLeft = getLast(left);
    pivot->previous = lastLeft;
    lastLeft->next = pivot;

    return left;
}