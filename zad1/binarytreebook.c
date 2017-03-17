#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

#include "include/binarytreebook.h"

treeNode *insertToTree(treeNode *current, treeNode *toInsert, compareContacts compare) ;

treeNode *removeContactNode(treeNode *root, contactStr *contact) ;

void deleteNodeTree(treeNode **root) ;

treeNode *findContactNodeByPhone(treeNode *node, char *phone) ;

treeNode *findContactNodeByEmail(treeNode *node, char *email) ;

void sortBy(bTBook *book, treeNode *toInsert, compareContacts getGreaterOrEq) ;

treeNode *inOrderSuccessor(treeNode *node) ;

void printNode(treeNode *node) ;

bTBook *createBTBook() {
    bTBook *book = malloc(sizeof(*book));
    book->root = NULL;
    return book;
}

void deleteBTBook(bTBook **pointer) {
    bTBook *pBook = *pointer;
    deleteNodeTree(&pBook->root);

    *pointer = NULL;
    free(pBook);
}

void addContactToBTBook(bTBook *book, contactStr *contact) {
    treeNode *node = malloc(sizeof(*node));
    node->contact = contact;
    book->root = insertToTree(book->root, node, &compareBySurname);
}

void deleteContactBTBook(bTBook *book, contactStr **contact) {
    book->root = removeContactNode(book->root, *contact);
    deleteContact(contact);
}

contactStr *findContactByPhoneBTBook(bTBook *book, char *phone) {
    return findContactNodeByPhone(book->root, phone)->contact;
}

contactStr *findContactByEmailBTBook(bTBook *book, char *email) {
    return findContactNodeByEmail(book->root, email)->contact;
}

void sortBTBookBySurname(bTBook *book) {
    treeNode *root = book->root;
    book->root = NULL;
    sortBy(book, root, &compareBySurname);
}

void sortBTBookByName(bTBook *book) {
    treeNode *root = book->root;
    book->root = NULL;
    sortBy(book, root, &compareByName);
}

void sortBTBookByDateOfBirth(bTBook *book) {
    treeNode *root = book->root;
    book->root = NULL;
    sortBy(book, root, &compareByDateOfBirth);
}

void sortBTBookByEmail(bTBook *book) {
    treeNode *root = book->root;
    book->root = NULL;
    sortBy(book, root, &compareByEmail);
}

void sortBTBookByPhone(bTBook *book) {
    treeNode *root = book->root;
    book->root = NULL;
    sortBy(book, root, &compareByPhone);
}

void printBTBook(bTBook *book) {
    printNode(book->root);
}

treeNode *insertToTree(treeNode *current, treeNode *toInsert, compareContacts compare) {
    if (current == NULL) {
        toInsert->left = NULL;
        toInsert->right = NULL;
        return toInsert;
    } else {
        if (compare(toInsert->contact, current->contact) >= 0) {
            current->right = insertToTree(current->right, toInsert, compare);
        } else {
            current->left = insertToTree(current->left, toInsert, compare);
        }
    }
    return current;
}

treeNode *removeContactNode(treeNode *root, contactStr *contact) {
    if (root == NULL) {
        return NULL;
    }

    if (root->contact == contact) {
        if (root->left == NULL) {
            treeNode *tmp = root->right;
            free(root);
            return tmp;
        } else if (root->right == NULL) {
            treeNode *tmp = root->left;
            free(root);
            return tmp;
        }

        treeNode *successor = inOrderSuccessor(root);
        root->contact = successor->contact;

        root->right = removeContactNode(root->right, successor->contact);

    } else {
        root->left = removeContactNode(root->left, contact);
        root->right = removeContactNode(root->right, contact);
    }
    return root;
}

void deleteNodeTree(treeNode **root) {
    treeNode *pNode = *root;
    if (pNode == NULL) return;

    if (pNode->contact != NULL) {
        deleteContact(&pNode->contact);
    }

    deleteNodeTree(&pNode->left);
    deleteNodeTree(&pNode->right);

    *root = NULL;
    free(pNode);
}

treeNode *findContactNodeByPhone(treeNode *node, char *phone) {
    if (node == NULL) return NULL;
    if (strcmp(node->contact->phone, phone) == 0) {
        return node;
    }

    treeNode *leftResult = findContactNodeByPhone(node->left, phone);
    if (leftResult != NULL) {
        return leftResult;
    }

    return findContactNodeByPhone(node->right, phone);
}

treeNode *findContactNodeByEmail(treeNode *node, char *email) {
    if (node == NULL) return NULL;
    if (strcmp(node->contact->email, email) == 0) {
        return node;
    }

    treeNode* leftResult = findContactNodeByEmail(node->left, email);
    if (leftResult != NULL) {
        return leftResult;
    }

    return findContactNodeByEmail(node->right, email);
}

void sortBy(bTBook *book, treeNode *toInsert, compareContacts getGreaterOrEq) {
    if (toInsert == NULL) return;
    treeNode *right = toInsert->right;
    treeNode *left = toInsert->left;
    toInsert->left = NULL;
    toInsert->right = NULL;

    sortBy(book, left, getGreaterOrEq);
    if (book->root == NULL) {
        book->root = toInsert;
    } else {
        insertToTree(book->root, toInsert, getGreaterOrEq);
    }
    sortBy(book, right, getGreaterOrEq);
}

treeNode *inOrderSuccessor(treeNode *node) {
    node = node->right;

    while (node->left != NULL) {
        node = node->left;
    }

    return node;
}

void printNode(treeNode *node) {
    if (node == NULL) {
        return;
    } else {
        printNode(node->left);
        printContact(node->contact);
        printNode(node->right);
    }
}
