#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/resource.h>
#include "include/t_measurement.h"

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

void make_time_measurement(micro_t_span *span, char *note) {
    printf("%s\treal:%.3f\tuser:%.3f\tsystem:%.3f\n", note, span->rtime, span->utime, span->stime);
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