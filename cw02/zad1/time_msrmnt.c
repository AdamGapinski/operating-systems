#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/resource.h>
#include "include/time_msrmnt.h"

const int SEC_TO_MICRO = 1000000;
const int SEC_TO_NANO = 1000000000;

time_span *create_time_span(timePoint *start, timePoint *end) {
    time_span *result = malloc(sizeof(*result));

    double rtime, utime, stime;

    /*setting utime and stime*/
    utime = end->utime->tv_usec - start->utime->tv_usec;
    utime /= SEC_TO_MICRO;  //converting microseconds to seconds
    stime = end->stime->tv_usec - start->stime->tv_usec;
    stime /= SEC_TO_MICRO;  //converting microseconds to seconds
    utime += end->utime->tv_sec - start->utime->tv_sec;
    stime += end->stime->tv_sec - start->stime->tv_sec;

    result->utime = utime;
    result->stime = stime;

    /*setting rtime*/
    rtime = end->rtime->tv_nsec - start->rtime->tv_nsec;
    rtime /= SEC_TO_NANO; //converting nanoseconds to seconds
    rtime += end->rtime->tv_sec - start->rtime->tv_sec;
    result->rtime = rtime;

    return result;
}

void delete_t_span(time_span *span) {
    free(span);
}

void make_time_measurement(time_span *span, char *note) {
    printf("%s\treal:%.6fs\tuser:%.6fs\tsystem:%.6fs\n\n", note, span->rtime, span->utime, span->stime);
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