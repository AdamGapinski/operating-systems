#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>


void save_data(FILE *fifo, int array_size);

int scale_to_row(double im, int array_size);

int scale_to_column(double r, int array_size);

void run_gnuplot(int array_size);

FILE *open_file(char *path, char *mode, char *err_msg) ;

int **alloc_array(int array_size) ;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments. Specify named pipe path and R integer - R x R array size\n");
        exit(EXIT_FAILURE);
    }
    char *path = argv[1];
    int array_size = 0;
    if (sscanf(argv[2], "%d", &array_size) != 1) {
        fprintf(stderr, "Error while parsing R parameter - R x R array size\n");
        exit(EXIT_FAILURE);
    }

    if (mkfifo(path, 0666) == -1) {
        perror("Error while creating fifo file");
        exit(EXIT_FAILURE);
    }
    FILE *fifo = open_file(path, "r", "Error while opening fifo file");
    save_data(fifo, array_size);
    fclose(fifo);
    run_gnuplot(array_size);
    getc(stdin);
}

void save_data(FILE *fifo, int array_size) {
    double r = 0.0;
    double im = 0.0;
    int iterations = 0;
    int **T = alloc_array(array_size);

    char *line_ptr = NULL;
    size_t line_len = 0;
    while (getline(&line_ptr, &line_len, fifo) != -1) {
        if (sscanf(line_ptr, "%lf %lf %d", &r, &im, &iterations) != 3) {
            fprintf(stderr, "Error while reading line %s from the fifo file\n", line_ptr);
            exit(EXIT_FAILURE);
        }

        T[scale_to_column(r, array_size)][scale_to_row(im, array_size)] = iterations;
    }
    free(line_ptr);

    FILE *data = open_file("data", "w", "Error while opening data file");
    for (int i = 0; i < array_size; ++i) {
        for (int j = 0; j < array_size; ++j) {
            fprintf(data, "%d %d %d\n", i, j, T[i][j]);
        }
        free(T[i]);
    }
    free(T);
    fclose(data);
}

int **alloc_array(int array_size) {
    int **result = malloc(sizeof(*result) * array_size);

    for (int i = 0; i < array_size; ++i) {
        result[i] = calloc((size_t) array_size, sizeof(*result[i]));
    }

    return result;
}

void run_gnuplot(int array_size) {
    const int INPUT_LINES = 4;
    FILE *fgnu = popen("gnuplot", "w");
    char *map[] = {
            "set view map\n",
            "set xrange [0:%d]\n",
            "set yrange [0:%d]\n",
            "plot 'data' with image\n"
    };

    for (int i = 0; i < INPUT_LINES; ++i) {
        fprintf(fgnu, map[i], array_size);
    }
    fflush(fgnu);
}

int scale_to_column(double r, int array_size) {
    const double SHIFT_RANGE = 2.0;
    const double REAL_RANGE = 3.0;
    int result = (int) (array_size * (r + SHIFT_RANGE) / REAL_RANGE);
    result = result < 0 ? 0 : result;
    return result >= array_size ? array_size - 1 : result;
}

int scale_to_row(double im, int array_size) {
    const double SHIFT_RANGE = 1.0;
    const double IMG_RANGE = 2.0;
    int result = (int) (array_size * (im + SHIFT_RANGE) / IMG_RANGE);
    result = result < 0 ? 0 : result;
    return result >= array_size ? array_size - 1 : result;
}

FILE *open_file(char *path, char *mode, char *err_msg) {
    FILE *result;
    if ((result = fopen(path, mode)) == NULL) {
        perror(err_msg);
        exit(EXIT_FAILURE);
    }
    return result;
}
