#ifndef PLIKI_TIME_MEASUREMENT_H
#define PLIKI_TIME_MEASUREMENT_H

typedef struct time_span {
    double rtime;
    double utime;
    double stime;
} time_span;

typedef struct timePoint {
    struct timespec *rtime;
    struct timeval *stime;
    struct timeval *utime;
} timePoint;

time_span *create_time_span(timePoint *start, timePoint *end);
void delete_t_span(time_span *span);
void make_time_measurement(time_span *span, char *note);
timePoint *createTimePoint();
void deleteTimePoint(timePoint *point);

#endif //PLIKI_TIME_MEASUREMENT_H
