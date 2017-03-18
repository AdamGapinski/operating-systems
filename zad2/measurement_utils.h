#ifndef ADDRESSBOOK_MEASUREMENT_UTILS_H
#define ADDRESSBOOK_MEASUREMENT_UTILS_H
#include "../zad1/include/linkedlistbook.h"
#include "../zad1/include/binarytreebook.h"
#include "t_measurement.h"

typedef linkedBook* (*on_contacts_m_linked)(contactStr **contacts);
typedef bTBook* (*on_contacts_m_tree)(contactStr **contacts);
typedef void (*on_linked_book_m)(linkedBook *book);
typedef void (*on_linked_book_ct_m)(linkedBook *book, contactStr *contact);
typedef void (*on_tree_book_m)(bTBook *book);
typedef void (*on_tree_book_m_ct_m)(bTBook *book, contactStr *contact);

micro_t_span *on_contacts_m_linked_time(on_contacts_m_linked method, contactStr **contacts);
micro_t_span *on_contacts_m_tree_time(on_contacts_m_tree method, contactStr **contacts);
micro_t_span *on_linked_book_m_time(on_linked_book_m method, linkedBook *book);
micro_t_span *on_linked_book_ct_m_time(on_linked_book_ct_m method, linkedBook *book, contactStr *contact);
micro_t_span *on_tree_book_m_time(on_tree_book_m method, bTBook *book);
micro_t_span *on_tree_book_ct_m_time(on_tree_book_m_ct_m method, bTBook *book, contactStr *contact);
#endif //ADDRESSBOOK_MEASUREMENT_UTILS_H
