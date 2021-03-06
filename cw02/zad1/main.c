#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "include/time_msrmnt.h"

typedef struct record {
    unsigned char first;
    size_t recordno;
    unsigned char *ptr;
    size_t size;
}record;

void generate(char *pathname, size_t records, size_t size);

void sort_lib(char *pathname, size_t size, size_t records) ;

void read_lib(unsigned char *buff, size_t size, FILE *fp) ;

void write_lib(unsigned char *buff, size_t size, FILE *fp) ;

void swap_records_lib(FILE *fp, record **first, record **second) ;

void shuffle_lib(char *pathname, size_t size, size_t records) ;

void sort_sys(char *pathname, size_t size, size_t records) ;

void read_sys(unsigned char *buff, size_t size, int fd);

void swap_records_sys(int fd, record **first, record **second);

void write_sys(unsigned char *buff, size_t size, int fd);

unsigned char* duplicate_record(unsigned char *ptr, size_t size) ;

void shuffle_sys(char *pathname, size_t size, size_t records) ;

void validate_file_info(char *filename, size_t size, size_t records) ;

record *create_record(unsigned char first, size_t record_num, unsigned char *ptr, size_t size) {
    record *result = malloc(sizeof(*result));
    result->first = first;
    result->recordno = record_num;
    result->ptr = duplicate_record(ptr, size);
    result->size = size;
    return result;
}

unsigned char* duplicate_record(unsigned char *ptr, size_t size) {
    unsigned char* result = malloc(size * sizeof(*result));

    for (int i = 0; i < size; ++i) {
        result[i] = ptr[i];
    }

    return result;
}

void delete_record(record *record) {
    free(record->ptr);
    free(record);
}

void swap(unsigned long *a, unsigned long *b) {
    unsigned long tmp = *a;
    *a = *b;
    *b = tmp;
}

void write_record_lib(FILE *fp, record *record) {
    long prev_offset = ftell(fp);
    size_t offset = record->recordno * record->size;

    if(fseek(fp, offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    write_lib(record->ptr, record->size, fp);

    if(fseek(fp, prev_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void swap_records(record **first, record **second) {
    record *tmp = *first;

    *first = *second;
    *second = tmp;

    swap(&(*first)->recordno, &(*second)->recordno);
}

typedef void (*operation_method)(char*, size_t, size_t) ;

time_span *measure_time (operation_method method, char *filename, size_t size, size_t records) {
    timePoint *start = createTimePoint();
    method(filename, size, records);
    timePoint *end = createTimePoint();

    time_span *result = create_time_span(start, end);
    deleteTimePoint(start);
    deleteTimePoint(end);
    return result;
}

int main(int argc, char **argv) {
    char *set = "";
    char *operation = "";
    char *filename = "";
    size_t records = 0;
    size_t size = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "sys") == 0) {
            set = "sys";
        } else if (strcmp(argv[i], "lib") == 0) {
            set = "lib";
        } else if (strcmp(argv[i], "generate") == 0) {
            operation = "generate";
        } else if (strcmp(argv[i], "shuffle") == 0) {
            operation = "shuffle";
        } else if (strcmp(argv[i], "sort") == 0) {
            operation = "sort";
        } else {
            if (strcmp(operation, "" ) == 0) {
                fprintf(stderr, "Error: Specify operation before file info.\n");
                exit(EXIT_FAILURE);
            }
            else if (strcmp(filename, "") == 0) {
                filename = argv[i];
            } else if (records != 0) {
                size = (size_t) atoi(argv[i]);
            } else {
                records = (size_t) atoi(argv[i]);
            }
        }
    }

    if (strcmp(operation, "sort") == 0) {
        validate_file_info(filename, size, records);

        if (strcmp(set, "lib") == 0) {
            time_span *span = measure_time(sort_lib, filename, size, records);
            make_time_measurement(span, "Library file functions:\n");
            delete_t_span(span);
        } else if (strcmp(set, "sys") == 0) {
            time_span *span = measure_time(sort_sys, filename, size, records);
            make_time_measurement(span, "System file functions:\n");
            delete_t_span(span);
        } else {
            fprintf(stderr, "Error: Method set not specified (sys or lib).\n");
            exit(EXIT_FAILURE);
        }

    } else if (strcmp(operation, "shuffle") == 0) {
        validate_file_info(filename, size, records);

        if (strcmp(set, "lib") == 0) {
            time_span *span = measure_time(shuffle_lib, filename, size, records);
            make_time_measurement(span, "Library file functions:\n");
            delete_t_span(span);

        } else if (strcmp(set, "sys") == 0) {
            time_span *span = measure_time(shuffle_sys, filename, size, records);
            make_time_measurement(span, "System file functions (in seconds):\n");
            delete_t_span(span);

        } else {
            fprintf(stderr, "Error: Method set not specified (sys or lib)\n");
            exit(EXIT_FAILURE);
        }

    } else if (strcmp(operation, "generate") == 0) {
        validate_file_info(filename, size, records);
        generate(filename, size, records);

    } else {
        fprintf(stderr, "Error: operation not specified.\n");
        return EXIT_FAILURE;
    }

    return 0;
}

void validate_file_info(char *filename, size_t size, size_t records) {
    if (strcmp(filename, "") == 0) {
        fprintf(stderr, "File name not specified.\n");
        exit(EXIT_FAILURE);
    }

    if (records <= 0 || size <= 0) {
        fprintf(stderr, "Number of records or size not specified (has to be greater than 0)\n");
        exit(EXIT_FAILURE);
    }
}

void generate(char *pathname, size_t size, size_t records) {
    FILE *rand = fopen("/dev/urandom", "r");
    FILE *generated = fopen(pathname, "w");
    unsigned char *buff = calloc(size, sizeof(*buff));

    for (size_t i = 0; i < records; ++i) {
        read_lib(buff, size, rand);
        write_lib(buff, size, generated);
    }
    free(buff);

    fclose(rand);
    fclose(generated);
}

void sort_lib(char *pathname, size_t size, size_t records) {
    FILE *fp = fopen(pathname, "r+");
    if (fp == NULL) {
        perror("Error");
    }
    unsigned char *buff = calloc(size, sizeof(*buff));
    record *first;
    record *second;

    for (size_t i = 0; i < records; ++i) {
        fseek(fp, i * size, SEEK_SET);
        read_lib(buff, size, fp);
        first = create_record(buff[0], i, buff, size);

        for (size_t j = i + 1; j < records; ++j) {
            read_lib(buff, size, fp);
            second = create_record(buff[0], j, buff, size);

            if (first->first > second->first) {
                swap_records_lib(fp, &first, &second);
            }
            delete_record(second);
        }
        delete_record(first);
    }

    free(buff);
    fclose(fp);
}

void shuffle_lib(char *pathname, size_t size, size_t records) {
    FILE *fp = fopen(pathname, "r+");
    if (fp == NULL) {
        perror("Error");
    }

    unsigned char *buff = calloc(size, sizeof(*buff));
    record *first;
    record *second;

    srand48(time(NULL));
    for (size_t i = records - 1; i > 0; --i) {
        fseek(fp, i * size, SEEK_SET);
        read_lib(buff, size, fp);
        first = create_record(buff[0], i, buff, size);

        size_t j = (size_t)(drand48() * i);
        fseek(fp, j * size, SEEK_SET);
        read_lib(buff, size, fp);
        second = create_record(buff[0], j, buff, size);
        swap_records_lib(fp, &first, &second);

        delete_record(second);
        delete_record(first);
    }

    free(buff);
    fclose(fp);
}

void read_lib(unsigned char *buff, size_t size, FILE *fp) {
    if (fread(buff, size, 1, fp) != 1) {
        if (feof(fp) != 0) {
            fprintf(stderr, "Error: reached end of file.\n");
        } else if (ferror(fp) != 0) {
            fprintf(stderr, "Error: while reading the file.\n");
        } else {
            perror("Error");
        }
        exit(EXIT_FAILURE);
    }
}

void write_lib(unsigned char *buff, size_t size, FILE *fp) {
    if (fwrite(buff, sizeof(*buff), size, fp) != size) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

void swap_records_lib(FILE *fp, record **first, record **second) {
    swap_records(first, second);
    write_record_lib(fp, *first);
    write_record_lib(fp, *second);
}

void sort_sys(char *pathname, size_t size, size_t records) {
    int fd = open(pathname, O_RDWR);
    if (fd == -1) {
        perror("Error");
    }

    unsigned char *buff = calloc(size, sizeof(*buff));
    record *first;
    record *second;

    for (size_t i = 0; i < records; ++i) {
        lseek(fd, i * size, SEEK_SET);
        read_sys(buff, size, fd);
        first = create_record(buff[0], i, buff, size);

        for (size_t j = i + 1; j < records; ++j) {
            read_sys(buff, size, fd);
            second = create_record(buff[0], j, buff, size);

            if (first->first > second->first) {
                swap_records_sys(fd, &first, &second);
            }
            delete_record(second);
        }
        delete_record(first);
    }

    free(buff);
    close(fd);
}

void shuffle_sys(char *pathname, size_t size, size_t records) {
    int fd = open(pathname, O_RDWR);
    if (fd == -1) {
        perror("Error");
    }

    unsigned char *buff = calloc(size, sizeof(*buff));
    record *first;
    record *second;

    srand48(time(NULL));
    for (size_t i = records - 1; i > 0; --i) {
        lseek(fd, i * size, SEEK_SET);
        read_sys(buff, size, fd);
        first = create_record(buff[0], i, buff, size);

        size_t j = (size_t)(drand48() * i);
        lseek(fd, j * size, SEEK_SET);
        read_sys(buff, size, fd);
        second = create_record(buff[0], j, buff, size);
        swap_records_sys(fd, &first, &second);

        delete_record(second);
        delete_record(first);
    }

    free(buff);
    close(fd);
}

void write_record_sys(int fd, record *record) {
    long prev_offset = lseek(fd, 0, SEEK_CUR);
    size_t offset = record->recordno * record->size;

    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    };

    write_sys(record->ptr, record->size, fd);

    if (lseek(fd, prev_offset, SEEK_SET) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    };
}

void swap_records_sys(int fd, record **first, record **second) {
    swap_records(first, second);

    write_record_sys(fd, *first);
    write_record_sys(fd, *second);
}

void read_sys(unsigned char *buff, size_t size, int fd) {
    if (read(fd, buff, size) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

void write_sys(unsigned char *buff, size_t size, int fd) {
    if (write(fd, buff, size) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

