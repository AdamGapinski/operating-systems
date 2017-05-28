#ifndef THREADSSYNCHRONIZATION_UTILS_H
#define THREADSSYNCHRONIZATION_UTILS_H

#define CLIENT_MAX_NAME 64

char *parseTextArg(int argc, char **argv, int arg_num, char *des);
int parseUnsignedIntArg(int argc, char **argv, int arg_num, char *des);
void setSigIntHandler(void (*handler)(int));
long get_thread_id();
void make_log(char *logArg, int var);

#endif //THREADSSYNCHRONIZATION_UTILS_H
