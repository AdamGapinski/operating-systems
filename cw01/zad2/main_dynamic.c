#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>
#include "include/t_measurement.h"
#include "../zad1/include/structures.h"

const int MAX_LINE_LENGTH = 200;
const int CONTACT_STR_FIELDS = 9;
const int RECORDS = 1000;

void measure_linkbook_creation(contactStr **contacts);

void measure_treebook_creation(contactStr **contacts);

void measure_linkbook_add_el(contactStr **pStr);

void measure_treebook_add_el(contactStr **pStr);

void measure_linkbook_sorting(contactStr **Book);

void measure_treebook_sorting(contactStr **book);

void measure_linkbook_finding_el(contactStr **pBook);

void measure_treebook_finding_el(contactStr **pBook);

void measure_linkbook_delete_el(contactStr **book);

void measure_treebook_delete_el(contactStr **book);

short *parseDate(char *string);

void parseContactLine(char *line, contactStr **contacts, int lineNum);

bTBook *create_treebook(contactStr **contacts) ;

linkedBook *create_linked_book(contactStr **contacts) ;

contactStr **load_data() ;

void delete_data(contactStr **contacts) ;

void* open_linkbook_lib_functions();

void* open_treebook_lib_functions();

void close_lib(void *handler) ;

int main() {
    void *handler = open_linkbook_lib_functions();

    contactStr **contacts = load_data();
    measure_linkbook_creation(contacts);
    measure_linkbook_add_el(contacts);
    measure_linkbook_delete_el(contacts);
    measure_linkbook_finding_el(contacts);
    measure_linkbook_sorting(contacts);
    close_lib(handler);

    printf("\n");

    handler = open_treebook_lib_functions();
    measure_treebook_creation(contacts);
    measure_treebook_add_el(contacts);
    measure_treebook_delete_el(contacts);
    measure_treebook_finding_el(contacts);
    measure_treebook_sorting(contacts);

    delete_data(contacts);
    close_lib(handler);

    return 0;
}

contactStr *(*createContact)(char*, char*, char*, char*, short, short, short, char*, char*, char*, char*);
void (*deleteContact)(contactStr**);
linkedBook* (*createLinkedBook)();
void (*deleteLinkedBook)(linkedBook**);
void (*addContactToLinkedBook)(linkedBook*, contactStr*);
void (*deleteContactLinkedBook)(linkedBook*, contactStr**);
contactStr *(*findContactByEmailLBook)(linkedBook*, char*);
void (*sortLinkedBookBySurname)(linkedBook*);
void (*sortLinkedBookByEmail)(linkedBook*);

bTBook* (*createBTBook)();
void (*deleteBTBook)(bTBook**);
void (*addContactToBTBook)(bTBook*, contactStr*);
void (*deleteContactBTBook)(bTBook*, contactStr**);
contactStr* (*findContactByEmailBTBook)(bTBook*, char*);
void (*sortBTBookBySurname)(bTBook*);
void (*sortBTBookByEmail)(bTBook*);

void *open_function(void *handle, char *function_name) {
    char *err;
    void *result = dlsym(handle, function_name);

    err = dlerror();
    if (err != NULL) {
        fprintf(stderr, "Error: %s\n", err);
        exit(EXIT_FAILURE);
    }

    return result;
}

void *open_linkbook_lib_functions() {
    void *handler = dlopen("../zad1/libaddressbook.so", RTLD_LAZY);
    if (handler == NULL) {
        fprintf(stderr, "Error: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    createLinkedBook = (linkedBook* (*)())
            open_function(handler, "createLinkedBook");
    deleteLinkedBook = (void (*)(linkedBook**))
            open_function(handler, "deleteLinkedBook");
    addContactToLinkedBook = (void (*)(linkedBook*, contactStr*))
            open_function(handler, "addContactToLinkedBook");
    deleteContactLinkedBook = (void (*)(linkedBook*, contactStr**))
            open_function(handler, "deleteContactLinkedBook");
    findContactByEmailLBook = (contactStr *(*)(linkedBook*, char*))
            open_function(handler, "findContactByEmailLBook");
    sortLinkedBookBySurname = (void (*)(linkedBook*))
            open_function(handler, "sortLinkedBookBySurname");
    sortLinkedBookByEmail = (void (*)(linkedBook*))
            open_function(handler, "sortLinkedBookByEmail");
    createContact = (contactStr *(*)(char*, char*, char*, char*, short, short, short, char*, char*, char*, char*))
            open_function(handler, "createContact");
    deleteContact = (void (*)(contactStr**))
            open_function(handler, "deleteContact");
    return handler;
}

void close_lib(void *handler) {
    dlclose(handler);
}

void *open_treebook_lib_functions() {
    void *handler = dlopen("../zad1/libaddressbook.so", RTLD_LAZY);
    if (handler == NULL) {
        fprintf(stderr, "Error: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    createBTBook = (bTBook *(*)())
            open_function(handler, "createBTBook");
    deleteBTBook = (void (*)(bTBook **))
            open_function(handler, "deleteBTBook");
    addContactToBTBook = (void (*)(bTBook *, contactStr *))
            open_function(handler, "addContactToBTBook");
    deleteContactBTBook = (void (*)(bTBook *, contactStr **))
            open_function(handler, "deleteContactBTBook");
    findContactByEmailBTBook = (contactStr *(*)(bTBook *, char *))
            open_function(handler, "findContactByEmailBTBook");
    sortBTBookBySurname = (void (*)(bTBook *))
            open_function(handler, "sortBTBookBySurname");
    sortBTBookByEmail = (void (*)(bTBook *))
            open_function(handler, "sortBTBookByEmail");
    createContact = (contactStr *(*)(char*, char*, char*, char*, short, short, short, char*, char*, char*, char*))
            open_function(handler, "createContact");
    deleteContact = (void (*)(contactStr**))
            open_function(handler, "deleteContact");

    return handler;
}

contactStr **load_data() {
    FILE *fp = fopen("../zad2/data", "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*
     * buffer is MAX_LINE_LENGTH + 1 length, because of null character to terminate the string.
     * */
    char *buff = calloc(MAX_LINE_LENGTH + 1, sizeof(*buff));
    contactStr **contacts = malloc(sizeof(*contacts) * RECORDS);

    buff = fgets(buff, MAX_LINE_LENGTH + 1, fp);
    int lineNum = 0;
    while(buff != NULL) {
        parseContactLine(buff, contacts, lineNum);
        lineNum += 1;
        buff = fgets(buff, MAX_LINE_LENGTH + 1, fp);
    }

    free(buff);
    fclose(fp);

    return contacts;
}

void delete_data(contactStr **contacts) {
    for (int i = 0; i < RECORDS; ++i) {
        deleteContact(&contacts[i]);
    }
    free(contacts);
}

linkedBook *create_linked_book(contactStr **contacts) {
    linkedBook *book = createLinkedBook();

    for(int i = 0; i < RECORDS; ++i) {
        addContactToLinkedBook(book, contacts[i]);
    }

    return book;
}

typedef linkedBook* (*on_contacts_m_linked)(contactStr **contacts);
typedef bTBook* (*on_contacts_m_tree)(contactStr **contacts);
typedef void (*on_linked_book_m)(linkedBook *book);
typedef void (*on_linked_book_ct_m)(linkedBook *book, contactStr *contact);
typedef void (*on_tree_book_m)(bTBook *book);
typedef void (*on_tree_book_m_ct_m)(bTBook *book, contactStr *contact);

micro_t_span *on_contacts_m_linked_time(on_contacts_m_linked method, contactStr **contacts) {
    timePoint *start = createTimePoint();
    linkedBook *book = method(contacts);
    timePoint *end = createTimePoint();

    micro_t_span *result = create_time_span(start, end);
    deleteLinkedBook(&book);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_contacts_m_tree_time(on_contacts_m_tree method, contactStr **contacts) {
    timePoint *start = createTimePoint();
    bTBook *book = method(contacts);
    timePoint *end = createTimePoint();

    micro_t_span *result = create_time_span(start, end);
    deleteBTBook(&book);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_linked_book_m_time(on_linked_book_m method, linkedBook *book) {
    timePoint *start = createTimePoint();
    method(book);
    timePoint *end = createTimePoint();

    micro_t_span *result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_linked_book_ct_m_time(on_linked_book_ct_m method, linkedBook *book, contactStr *contact) {
    timePoint *start = createTimePoint();
    method(book, contact);
    timePoint *end = createTimePoint();

    micro_t_span *result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_tree_book_m_time(on_tree_book_m method, bTBook *book) {
    timePoint *start = createTimePoint();
    method(book);
    timePoint *end = createTimePoint();

    micro_t_span *result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_tree_book_ct_m_time(on_tree_book_m_ct_m method, bTBook *book, contactStr *contact) {
    timePoint *start = createTimePoint();
    method(book, contact);
    timePoint *end = createTimePoint();

    micro_t_span *result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

void measure_linkbook_creation(contactStr **contacts) {
    micro_t_span *span1 = on_contacts_m_linked_time(&create_linked_book, contacts);
    micro_t_span *span2 = on_contacts_m_linked_time(&create_linked_book, contacts);
    micro_t_span *span3 = on_contacts_m_linked_time(&create_linked_book, contacts);

    micro_t_span *avg = calc_average(span1, span2, span3);

    make_time_measurement(avg, "Linked address book with 1000 records creation time in microseconds:\n\t");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);
}

void measure_linkbook_add_el(contactStr **contacts) {
    linkedBook *book1 = createLinkedBook();
    linkedBook *book2 = createLinkedBook();
    linkedBook *book3 = createLinkedBook();
    printf("Single elements additions to linked list address book times in microseconds:\n");

    micro_t_span *span1;
    micro_t_span *span2;
    micro_t_span *span3;
    micro_t_span *avg;
    for(int i = 0; i < RECORDS; ++i) {
        span1 = on_linked_book_ct_m_time(addContactToLinkedBook, book1, contacts[i]);
        span2 = on_linked_book_ct_m_time(addContactToLinkedBook, book2, contacts[i]);
        span3 = on_linked_book_ct_m_time(addContactToLinkedBook, book3, contacts[i]);

        avg = calc_average(span1, span2, span3);

        printf("element %d:", i);
        make_time_measurement(avg, "");

        delete_t_span(span1);
        delete_t_span(span2);
        delete_t_span(span3);
        delete_t_span(avg);
    }
    deleteLinkedBook(&book1);
    deleteLinkedBook(&book2);
    deleteLinkedBook(&book3);
}

void find_contact_linked(linkedBook *book, contactStr *contact) {
    findContactByEmailLBook(book, contact->email);
}

void measure_linkbook_finding_el(contactStr **contacts) {
    linkedBook *book = create_linked_book(contacts);
    sortLinkedBookByEmail(book);
    printf("Time of finding element in linked list address book in microseconds:\n");

    contactStr *to_find = findContactByEmailLBook(book, "a.arcu.Sed@InfaucibusMorbi.edu");
    micro_t_span *span1 = on_linked_book_ct_m_time(&find_contact_linked, book, to_find);
    micro_t_span *span2 = on_linked_book_ct_m_time(&find_contact_linked, book, to_find);
    micro_t_span *span3 = on_linked_book_ct_m_time(&find_contact_linked, book, to_find);
    micro_t_span *avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "optimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);


    to_find = findContactByEmailLBook(book, "vulputate@penatibuset.co.uk");
    span1 = on_linked_book_ct_m_time(&find_contact_linked, book, to_find);
    span2 = on_linked_book_ct_m_time(&find_contact_linked, book, to_find);
    span3 = on_linked_book_ct_m_time(&find_contact_linked, book, to_find);
    avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "pessimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);

    deleteLinkedBook(&book);
}

contactStr *get_added(linkedBook *book, contactStr *contact) {
    addContactToLinkedBook(book, contact);
    sortLinkedBookByEmail(book);
    return findContactByEmailLBook(book, contact->email);
}

void delete_contact_linked_book(linkedBook *book, contactStr *to_delete) {
    deleteContactLinkedBook(book, &to_delete);
}

void measure_linkbook_delete_el(contactStr **contacts) {
    linkedBook *book = create_linked_book(contacts);

    char optimisticmail[] = "a.aa@email.com";
    contactStr *to_delete = createContact("test", "test", optimisticmail, "test", 0, 0, 0, "test", "test", "test", "tes");

    printf("Time of deleting element from linked list address book in microseconds:\n");

    micro_t_span *span1 = on_linked_book_ct_m_time(&delete_contact_linked_book, book, get_added(book, to_delete));
    micro_t_span *span2 = on_linked_book_ct_m_time(&delete_contact_linked_book, book, get_added(book, to_delete));
    micro_t_span *span3 = on_linked_book_ct_m_time(&delete_contact_linked_book, book, get_added(book, to_delete));

    micro_t_span *avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "optimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);

    deleteContact(&to_delete);


    char pessimisticmail[] = "ZZ@mail.com";
    to_delete = createContact("test", "test", pessimisticmail, "test", 0, 0, 0, "test", "test", "test", "test");

    span1 = on_linked_book_ct_m_time(&delete_contact_linked_book, book, get_added(book, to_delete));
    span2 = on_linked_book_ct_m_time(&delete_contact_linked_book, book, get_added(book, to_delete));
    span3 = on_linked_book_ct_m_time(&delete_contact_linked_book, book, get_added(book, to_delete));

    avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "pessimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);

    deleteContact(&to_delete);
    deleteLinkedBook(&book);
}

void measure_linkbook_sorting(contactStr **contacts) {
    linkedBook *book = create_linked_book(contacts);

    sortLinkedBookBySurname(book);
    micro_t_span *span1 = on_linked_book_m_time(sortLinkedBookByEmail, book);

    sortLinkedBookBySurname(book);
    micro_t_span *span2 = on_linked_book_m_time(sortLinkedBookByEmail, book);

    sortLinkedBookBySurname(book);
    micro_t_span *span3 = on_linked_book_m_time(sortLinkedBookByEmail, book);

    micro_t_span *avg = calc_average(span1,span2,span3);


    make_time_measurement(avg, "Time of sorting elements in linked address book in microseconds: \n\t");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);
    deleteLinkedBook(&book);
}

bTBook *create_treebook(contactStr **contacts) {
    bTBook *book = createBTBook();

    for(int i = 0; i < RECORDS; ++i) {
        addContactToBTBook(book, contacts[i]);
    }

    return book;
}

void measure_treebook_creation(contactStr **contacts) {
    micro_t_span *span1 = on_contacts_m_tree_time(&create_treebook, contacts);
    micro_t_span *span2 = on_contacts_m_tree_time(&create_treebook, contacts);
    micro_t_span *span3 = on_contacts_m_tree_time(&create_treebook, contacts);

    micro_t_span *avg = calc_average(span1, span2, span3);

    make_time_measurement(avg, "Binary tree address book with 1000 records creation time in microseconds:\n\t");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);
}


void measure_treebook_add_el(contactStr **contacts) {
    bTBook *book1 = createBTBook();
    bTBook *book2 = createBTBook();
    bTBook *book3 = createBTBook();
    printf("Single elements additions to binary tree address book times in microseconds:\n");
    for(int i = 0; i < RECORDS; ++i) {
        micro_t_span *span1 = on_tree_book_ct_m_time(addContactToBTBook, book1, contacts[i]);
        micro_t_span *span2 = on_tree_book_ct_m_time(addContactToBTBook, book2, contacts[i]);
        micro_t_span *span3 = on_tree_book_ct_m_time(addContactToBTBook, book3, contacts[i]);

        micro_t_span *avg = calc_average(span1, span2, span3);

        printf("element %d:", i);
        make_time_measurement(avg, "");

        delete_t_span(span1);
        delete_t_span(span2);
        delete_t_span(span3);
        delete_t_span(avg);
    }
    deleteBTBook(&book1);
    deleteBTBook(&book2);
    deleteBTBook(&book3);
}

void find_contact_tree(bTBook *book, contactStr *contact) {
    findContactByEmailBTBook(book, contact->email);
}

void measure_treebook_finding_el(contactStr **contacts) {
    bTBook *book = create_treebook(contacts);
    sortBTBookByEmail(book);
    printf("Time of finding element in binary tree address book in microseconds:\n");

    contactStr *to_find = findContactByEmailBTBook(book, "Morbi@sempertellus.com");
    micro_t_span *span1 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    micro_t_span *span2 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    micro_t_span *span3 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    micro_t_span *avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "optimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);

    sortBTBookByEmail(book);
    to_find = findContactByEmailBTBook(book, "vulputate@penatibuset.co.uk");
    span1 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    span2 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    span3 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "pessimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);

    deleteBTBook(&book);
}

void delete_contact_tree_book(bTBook *book, contactStr *contact) {
    deleteContactBTBook(book, &contact);
}

void measure_treebook_delete_el(contactStr **contacts) {
    bTBook *book1 = create_treebook(contacts);
    bTBook *book2 = create_treebook(contacts);
    bTBook *book3 = create_treebook(contacts);
    sortBTBookByEmail(book1);
    sortBTBookByEmail(book1);
    sortBTBookByEmail(book2);
    sortBTBookByEmail(book2);
    sortBTBookByEmail(book3);
    sortBTBookByEmail(book3);

    char pessimisticmail[] = "vulputate@penatibuset.co.uk";

    micro_t_span *span1 = on_tree_book_ct_m_time(&delete_contact_tree_book, book1, findContactByEmailBTBook(book1, pessimisticmail));
    micro_t_span *span2 = on_tree_book_ct_m_time(&delete_contact_tree_book, book2, findContactByEmailBTBook(book2, pessimisticmail));
    micro_t_span *span3 = on_tree_book_ct_m_time(&delete_contact_tree_book, book3, findContactByEmailBTBook(book3, pessimisticmail));
    micro_t_span *avg_pess = calc_average(span1, span2, span3);

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);

    char optimisticmail[] = "a.arcu.Sed@InfaucibusMorbi.edu";

    span1 = on_tree_book_ct_m_time(&delete_contact_tree_book, book1, findContactByEmailBTBook(book1, optimisticmail));
    span2 = on_tree_book_ct_m_time(&delete_contact_tree_book, book2, findContactByEmailBTBook(book2, optimisticmail));
    span3 = on_tree_book_ct_m_time(&delete_contact_tree_book, book3, findContactByEmailBTBook(book3, optimisticmail));
    micro_t_span *avg_opt = calc_average(span1, span2, span3);

    printf("Time of deleting element from binary tree address book in microseconds:\n");

    make_time_measurement(avg_opt, "optimistic: ");
    make_time_measurement(avg_pess, "pessimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg_pess);
    delete_t_span(avg_opt);

    deleteBTBook(&book1);
    deleteBTBook(&book2);
    deleteBTBook(&book3);
}

void measure_treebook_sorting(contactStr **contacts) {
    bTBook *book = create_treebook(contacts);

    sortBTBookBySurname(book);
    micro_t_span *span1 = on_tree_book_m_time(sortBTBookByEmail, book);

    sortBTBookBySurname(book);
    micro_t_span *span2 = on_tree_book_m_time(sortBTBookByEmail, book);

    sortBTBookBySurname(book);
    micro_t_span *span3 = on_tree_book_m_time(sortBTBookByEmail, book);

    micro_t_span *avg = calc_average(span1,span2,span3);


    make_time_measurement(avg, "Time of sorting elements in binary tree address book in microseconds:\n\t");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);
    deleteBTBook(&book);
}

void parseContactLine(char *line, contactStr **contacts, int lineNum) {
    line = strdup(line);
    char *linePtr = line;

    char **fields = calloc(CONTACT_STR_FIELDS, sizeof(*fields));

    line = strtok(line, "|");
    int fieldNum = 0;
    while (line != NULL) {
        fields[fieldNum++] = strdup(line);
        line = strtok(NULL, "|\n");
    }

    short *parsedDate = parseDate(fields[4]);
    contacts[lineNum] = createContact(fields[0], fields[1], fields[2], fields[3],
                                                parsedDate[0], parsedDate[1], parsedDate[2],
                                                fields[5], fields[6], fields[7], fields[8]);

    for (int i = 0; i < CONTACT_STR_FIELDS; ++i) {
        free(fields[i]);
    }
    free(linePtr);
    free(fields);
    free(parsedDate);
}

short *parseDate(char *string) {
    char *line = strdup(string);
    void *linePtr = line;
    short *parsedDate = malloc(sizeof(*parsedDate) * 3);

    line = strtok(line, "/");
    int i = 0;
    while(line != NULL) {
        parsedDate[i++] = (short) atoi(line);
        line = strtok(NULL, "/");
    }

    free(linePtr);
    return parsedDate;
}
