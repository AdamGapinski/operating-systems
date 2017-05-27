#ifndef THREADSSYNCHRONIZATION_UTILS_H
#define THREADSSYNCHRONIZATION_UTILS_H

char *parseTextArg(int argc, char **argv, int arg_num, char *des);
int parseUnsignedIntArg(int argc, char **argv, int arg_num, char *des);
void setSigIntHandler(void (*handler)(int));
long get_thread_id();
void make_log(char *logArg, int var);

#endif //THREADSSYNCHRONIZATION_UTILS_H
