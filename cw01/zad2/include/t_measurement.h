#ifndef ADDRESSBOOK_T_MEASUREMENT_H
#define ADDRESSBOOK_T_MEASUREMENT_H

typedef struct micro_t_span {
    double rtime;
    double utime;
    double stime;
} micro_t_span;

typedef struct timePoint {
    struct timespec *rtime;
    struct timeval *stime;
    struct timeval *utime;
} timePoint;

micro_t_span *create_time_span(timePoint *start, timePoint *end);
void delete_t_span(micro_t_span *span);
micro_t_span *calc_average(micro_t_span *fst, micro_t_span *scd, micro_t_span *lst);
void make_time_measurement(micro_t_span *span, char *note);
timePoint *createTimePoint();
void deleteTimePoint(timePoint *point);

#endif //ADDRESSBOOK_T_MEASUREMENT_H
