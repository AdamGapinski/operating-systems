#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include "../zad1/include/linkedlistbook.h"
#include "../zad1/include/binarytreebook.h"

const int MAX_LINE_LENGTH = 200;
const int CONTACT_STR_FIELDS = 9;
const int RECORDS = 1000;

short *parseDate(char *string);

void parseContactLine(char *line, contactStr **contacts, int lineNum) ;

void measure_link_creation(contactStr **contacts) ;

void measure_treebook_creation(contactStr **contacts);

void measure_link_add_el(contactStr **pStr);

void measure_treebook_add_el(contactStr **pStr);

void measure_link_sorting(contactStr **Book);

void measure_treebook_sorting(contactStr **book) ;

void measure_link_finding_el(contactStr **pBook);

void measure_treebook_finding_el(contactStr **pBook);

void measure_link_delete_el(contactStr **book) ;

void measure_treebook_delete_el(contactStr **book);

typedef struct micro_t_span {
    double rtime;
    double utime;
    double stime;
} micro_t_span;

typedef linkedBook* (*on_contacts_m_linked)(contactStr **contacts);
typedef bTBook* (*on_contacts_m_tree)(contactStr **contacts);
typedef void (*on_linked_book_m)(linkedBook *book);
typedef void (*on_linked_book_ct_m)(linkedBook *book, contactStr *contact);
typedef void (*on_tree_book_m)(bTBook *book);
typedef void (*on_tree_book_m_ct_m)(bTBook *book, contactStr *contact);

micro_t_span *on_linked_book_ct_m_time(on_linked_book_ct_m method, linkedBook *book, contactStr *contact) ;

linkedBook *create_linked_book(contactStr **contacts) ;

micro_t_span *on_tree_book_ct_m_time(on_tree_book_m_ct_m method, bTBook *book, contactStr *contact) ;

bTBook *create_treebook(contactStr **contacts) ;

micro_t_span *on_tree_book_m_time(on_tree_book_m method, bTBook *book) ;

typedef struct timePoint {
    struct timespec *rtime;
    struct timeval *stime;
    struct timeval *utime;
} timePoint;

micro_t_span *create_time_span(timePoint *start, timePoint *end) {
    const int SEC_TO_MICRO = 1000000;
    const int MICRO_TO_NANO = 1000;
    micro_t_span *result = malloc(sizeof(*result));

    double rtime, utime, stime;

    /*setting utime and stime*/
    utime = end->utime->tv_usec - start->utime->tv_usec;
    stime = end->stime->tv_usec - start->stime->tv_usec;
    utime += (end->utime->tv_sec - start->utime->tv_sec) * SEC_TO_MICRO; //converting seconds to microseconds
    stime += (end->stime->tv_sec - start->stime->tv_sec) * SEC_TO_MICRO; //converting seconds to microseconds
    result->utime = utime;
    result->stime = stime;

    /*setting rtime*/
    rtime = end->rtime->tv_nsec - start->rtime->tv_nsec;
    rtime /= MICRO_TO_NANO; //converting nanoseconds to microseconds
    rtime += (end->rtime->tv_sec - start->rtime->tv_sec) * SEC_TO_MICRO; //converting seconds to microseconds
    result->rtime = rtime;

    return result;
}

void delete_t_span(micro_t_span *span) {
    free(span);
}
micro_t_span *calc_average(micro_t_span *fst, micro_t_span *scd, micro_t_span *lst) {
    micro_t_span *result = malloc(sizeof(*result));
    double rtime, utime, stime;

    rtime = fst->rtime + scd->rtime + lst->rtime;
    rtime /= 3;

    utime = fst->utime + scd->utime + lst->utime;
    utime /= 3;

    stime = fst->stime + scd->stime + lst->stime;
    stime /= 3;

    result->rtime = rtime;
    result->utime = utime;
    result->stime = stime;

    return result;
}


timePoint *createTimePoint() {
    timePoint *point = malloc(sizeof(*point));
    point->rtime = malloc(sizeof(*point->rtime));
    point->stime = malloc(sizeof(*point->stime));
    point->utime = malloc(sizeof(*point->utime));
    clock_gettime(CLOCK_MONOTONIC, point->rtime);

    struct rusage buff_ust;
    getrusage(RUSAGE_SELF, &buff_ust);

    point->stime->tv_sec = buff_ust.ru_stime.tv_sec;
    point->stime->tv_usec = buff_ust.ru_stime.tv_usec;
    point->utime->tv_sec = buff_ust.ru_utime.tv_sec;
    point->utime->tv_usec = buff_ust.ru_utime.tv_usec;

    return point;
}

void deleteTimePoint(timePoint *point) {
    free(point->rtime);
    free(point->stime);
    free(point->utime);
    free(point);
}

void make_time_measurement(micro_t_span *span, char *note) {
    printf("%s\treal:%.3f\tuser:%.3f\tsys:%.3f\n", note, span->rtime, span->utime, span->stime);
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

int main() {
    contactStr **contacts = load_data();

    measure_link_creation(contacts);
    measure_link_add_el(contacts);
    measure_link_delete_el(contacts);
    measure_link_finding_el(contacts);
    measure_link_sorting(contacts);


    measure_treebook_creation(contacts);
    measure_treebook_add_el(contacts);
    measure_treebook_delete_el(contacts);
    measure_treebook_finding_el(contacts);
    measure_treebook_sorting(contacts);

    delete_data(contacts);
}

void find_contact_linked(linkedBook *book, contactStr *contact) {
    findContactByEmailLBook(book, contact->email);
}

void measure_link_finding_el(contactStr **contacts) {
    linkedBook *book = create_linked_book(contacts);
    sortLinkedBookByEmail(book);
    printf("Time of finding element in linked list address book in microseconds:\n");

    contactStr *to_find = findContactByEmailLBook(book, "Aenean.eget.metus@Nullamenim.co.uk");
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

void measure_link_delete_el(contactStr **contacts) {
    linkedBook *book = create_linked_book(contacts);

    char optimisticmail[] = "AA@email.com";
    contactStr *to_delete = createContact("test", "test", optimisticmail, "test", 0, 0, 0, "test", "test", "test", "tes");

    printf("Time of deleting element in linked list address book in microseconds:\n");

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

void find_contact_tree(bTBook *book, contactStr *contact) {
    findContactByEmailBTBook(book, contact->email);
}

void measure_treebook_finding_el(contactStr **contacts) {
    bTBook *book = create_treebook(contacts);
    sortBTBookByEmail(book);
    printf("Time of finding element in binary tree address book in microseconds:\n");

    contactStr *to_find = findContactByEmailBTBook(book, "Aenean.eget.metus@Nullamenim.co.uk");
    micro_t_span *span1 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    micro_t_span *span2 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    micro_t_span *span3 = on_tree_book_ct_m_time(&find_contact_tree, book, to_find);
    micro_t_span *avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "optimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);


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

contactStr *get_added_bintree(bTBook *book, contactStr *contact) {
    addContactToBTBook(book, contact);
    return findContactByEmailBTBook(book, contact->email);
}

void delete_contact_tree_book(bTBook *book, contactStr *contact) {
    deleteContactBTBook(book, &contact);
}

void measure_treebook_delete_el(contactStr **contacts) {
    bTBook *book = create_treebook(contacts);
    sortBTBookByEmail(book);

    char optimisticmail[] = "MMorbi@sempertellus.com";
    contactStr *to_delete = createContact("test", "test", optimisticmail, "test", 0, 0, 0, "test", "test", "test", "test");

    printf("Time of deleting element in binary tree address book in microseconds:\n");

    micro_t_span *span1 = on_tree_book_ct_m_time(&delete_contact_tree_book, book, get_added_bintree(book, to_delete));
    micro_t_span *span2 = on_tree_book_ct_m_time(&delete_contact_tree_book, book, get_added_bintree(book, to_delete));
    micro_t_span *span3 = on_tree_book_ct_m_time(&delete_contact_tree_book, book, get_added_bintree(book, to_delete));

    micro_t_span *avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "optimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);

    deleteContact(&to_delete);


    sortBTBookByEmail(book);
    char pessimisticmail[] = "vulputate@penatibuset.co.uk";
    to_delete = createContact("test", "test", pessimisticmail, "test", 0, 0, 0, "test", "test", "test", "test");

    span1 = on_tree_book_ct_m_time(&delete_contact_tree_book, book, get_added_bintree(book, to_delete));
    span2 = on_tree_book_ct_m_time(&delete_contact_tree_book, book, get_added_bintree(book, to_delete));
    span3 = on_tree_book_ct_m_time(&delete_contact_tree_book, book, get_added_bintree(book, to_delete));

    avg = calc_average(span1, span2, span3);
    make_time_measurement(avg, "pessimistic: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);

    deleteContact(&to_delete);
    deleteBTBook(&book);
}

void measure_treebook_sorting(contactStr **contacts) {
    bTBook *book = create_treebook(contacts);

    sortBTBookBySurname(book);
    micro_t_span *span1 = on_tree_book_m_time(&sortBTBookByEmail, book);

    sortBTBookBySurname(book);
    micro_t_span *span2 = on_tree_book_m_time(&sortBTBookByEmail, book);

    sortBTBookBySurname(book);
    micro_t_span *span3 = on_tree_book_m_time(&sortBTBookByEmail, book);

    micro_t_span *avg = calc_average(span1,span2,span3);


    make_time_measurement(avg, "Time of sorting elements in binary tree address book in microseconds: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);
    deleteBTBook(&book);
}

void measure_link_add_el(contactStr **contacts) {
    linkedBook *book = createLinkedBook();
    printf("Single element addition to linked list address book times in microseconds:\n");
    for(int i = 0; i < RECORDS; ++i) {
        micro_t_span *span1 = on_linked_book_ct_m_time(&addContactToLinkedBook, book, contacts[i]);
        micro_t_span *span2 = on_linked_book_ct_m_time(&addContactToLinkedBook, book, contacts[i]);
        micro_t_span *span3 = on_linked_book_ct_m_time(&addContactToLinkedBook, book, contacts[i]);

        micro_t_span *avg = calc_average(span1, span2, span3);

        printf("element %d:", i);
        make_time_measurement(avg, "");

        delete_t_span(span1);
        delete_t_span(span2);
        delete_t_span(span3);
        delete_t_span(avg);
    }
    deleteLinkedBook(&book);
}

void measure_treebook_add_el(contactStr **contacts) {
    bTBook *book = createBTBook();
    printf("Single element addition to binary tree address book times in microseconds:\n");
    for(int i = 0; i < RECORDS; ++i) {
        micro_t_span *span1 = on_tree_book_ct_m_time(&addContactToBTBook, book, contacts[i]);
        micro_t_span *span2 = on_tree_book_ct_m_time(&addContactToBTBook, book, contacts[i]);
        micro_t_span *span3 = on_tree_book_ct_m_time(&addContactToBTBook, book, contacts[i]);

        micro_t_span *avg = calc_average(span1, span2, span3);

        printf("element %d:", i);
        make_time_measurement(avg, "");

        delete_t_span(span1);
        delete_t_span(span2);
        delete_t_span(span3);
        delete_t_span(avg);
    }
    deleteBTBook(&book);
}

micro_t_span *on_contacts_m_linked_time(on_contacts_m_linked method, contactStr **contacts) {
    micro_t_span *result;
    timePoint *start = createTimePoint();

    linkedBook *book = method(contacts);

    timePoint *end = createTimePoint();
    result = create_time_span(start, end);

    deleteLinkedBook(&book);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_contacts_m_tree_time(on_contacts_m_tree method, contactStr **contacts) {
    micro_t_span *result;
    timePoint *start = createTimePoint();

    bTBook *book = method(contacts);

    timePoint *end = createTimePoint();
    result = create_time_span(start, end);

    deleteBTBook(&book);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_linked_book_m_time(on_linked_book_m method, linkedBook *book) {
    micro_t_span *result;
    timePoint *start = createTimePoint();

    method(book);

    timePoint *end = createTimePoint();
    result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_linked_book_ct_m_time(on_linked_book_ct_m method, linkedBook *book, contactStr *contact) {
    micro_t_span *result;
    timePoint *start = createTimePoint();

    method(book, contact);

    timePoint *end = createTimePoint();
    result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_tree_book_m_time(on_tree_book_m method, bTBook *book) {
    micro_t_span *result;
    timePoint *start = createTimePoint();

    method(book);

    timePoint *end = createTimePoint();
    result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

micro_t_span *on_tree_book_ct_m_time(on_tree_book_m_ct_m method, bTBook *book, contactStr *contact) {
    micro_t_span *result;
    timePoint *start = createTimePoint();

    method(book, contact);

    timePoint *end = createTimePoint();
    result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

linkedBook *create_linked_book(contactStr **contacts) {
    linkedBook *book = createLinkedBook();

    for(int i = 0; i < RECORDS; ++i) {
        addContactToLinkedBook(book, contacts[i]);
    }

    return book;
}
void measure_link_creation(contactStr **contacts) {
    micro_t_span *span1 = on_contacts_m_linked_time(&create_linked_book, contacts);
    micro_t_span *span2 = on_contacts_m_linked_time(&create_linked_book, contacts);
    micro_t_span *span3 = on_contacts_m_linked_time(&create_linked_book, contacts);

    micro_t_span *avg = calc_average(span1, span2, span3);

    make_time_measurement(avg, "Linked address book with 1000 records creation time in microseconds:");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);
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

    make_time_measurement(avg, "Binary tree address book with 1000 records creation time in microseconds:");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);
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

void measure_link_sorting(contactStr **contacts) {
    linkedBook *book = create_linked_book(contacts);

    sortLinkedBookBySurname(book);
    micro_t_span *span1 = on_linked_book_m_time(&sortLinkedBookByEmail, book);

    sortLinkedBookBySurname(book);
    micro_t_span *span2 = on_linked_book_m_time(&sortLinkedBookByEmail, book);

    sortLinkedBookBySurname(book);
    micro_t_span *span3 = on_linked_book_m_time(&sortLinkedBookByEmail, book);

    micro_t_span *avg = calc_average(span1,span2,span3);


    make_time_measurement(avg, "Time of sorting elements in linked address book in microseconds: ");

    delete_t_span(span1);
    delete_t_span(span2);
    delete_t_span(span3);
    delete_t_span(avg);
    deleteLinkedBook(&book);
}

