#ifndef THREADSSYNCHRONIZATION_UTILS_H
#define THREADSSYNCHRONIZATION_UTILS_H

#define INT_ARRAY_SIZE 20

int verbose;

int parseVerboseArg(int argc, char **argv);
int parseUnsignedIntArg(int argc, char **argv, int arg_num, char *des);
int *init_integer_array();
long get_thread_id();

#endif //THREADSSYNCHRONIZATION_UTILS_H
