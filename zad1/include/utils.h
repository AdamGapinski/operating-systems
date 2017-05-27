#ifndef THREADSSYNCHRONIZATION_UTILS_H
#define THREADSSYNCHRONIZATION_UTILS_H

char *parseTextArg(int argc, char **argv, int arg_num, char *des);
int parseUnsignedIntArg(int argc, char **argv, int arg_num, char *des);
long get_thread_id();

#endif //THREADSSYNCHRONIZATION_UTILS_H
