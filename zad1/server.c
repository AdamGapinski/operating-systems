#include "include/utils.h"

int main(int argc, char *argv[]) {
    int port = parseUnsignedIntArg(argc, argv, 1, "UDP port number");
    char *path = parseTextArg(argc, argv, 2, "Unix local socket path");

    return 0;
}
