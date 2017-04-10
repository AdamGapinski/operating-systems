#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <time.h>
#include <unistd.h>

int iterations(double r, double im, int iteration_limit) ;

void save_randoms(int random_points, FILE *fifo, int iteration_limit) ;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Wrong number of arguments. Specify named pipe path, N - number of random points and K - iteration limit\n");
        exit(EXIT_FAILURE);
    }
    char *path = argv[1];
    int random_points = 0;
    if (sscanf(argv[2], "%d", &random_points) != 1) {
        fprintf(stderr, "Error while parsing N parameter - number of random points\n");
        exit(EXIT_FAILURE);
    }
    int iteration_limit = 0;
    if (sscanf(argv[3], "%d", &iteration_limit) != 1) {
        fprintf(stderr, "Error while parsing K parameter - value of iteration limit\n");
        exit(EXIT_FAILURE);
    }
    FILE *fifo;
    if ((fifo = fopen(path, "a")) == NULL) {
        perror("Error while opening fifo file");
        exit(EXIT_FAILURE);
    }

    srand48((unsigned int) (time(NULL) ^ (getpid() << 16)));
    save_randoms(random_points, fifo, iteration_limit);
    fclose(fifo);
}

void save_randoms(int random_points, FILE *fifo, int iteration_limit) {
    for (int i = 0; i < random_points; ++i) {
        double r = drand48() * 3.0 - 2.0;
        double im = drand48() * 2.0 - 1.0;
        int iter = iterations(r, im, iteration_limit);
        fprintf(fifo, "%lf %lf %d\n", r, im, iter);
        fflush(fifo);
    }
}

int iterations(double r, double im, int iteration_limit) {
    int result = 0;
    double complex num = r + im * I;
    double complex fcz = 0;

    while (cabs(fcz) <= 2.0 && result < iteration_limit) {
        fcz = cpow(fcz, 2) + num;
        ++result;
    }

    return result;
}
