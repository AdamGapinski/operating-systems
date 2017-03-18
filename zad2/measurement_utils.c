#include "measurement_utils.h"

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