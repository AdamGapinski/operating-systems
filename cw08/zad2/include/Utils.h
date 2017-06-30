#ifndef THREADS_UTILS_H
#define THREADS_UTILS_H

#define _GNU_SOURCE
#define RECORD_SIZE 1024

int parseUnsignedIntArg(int argc, char **argv, int arg_num, char *des);
char *parseTextArg(int argc, char **argv, int arg_num, char *des);
int open_file(char *path);
long get_thread_id();
int parse_signal_name(char *sig);

#endif //THREADS_UTILS_H
