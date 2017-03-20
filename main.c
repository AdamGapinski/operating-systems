#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <time.h>

typedef struct record {
    unsigned char first;
    size_t recordno;
    unsigned char *ptr;
    size_t size;
}record;

void generate(char *filename, size_t records, size_t size);

void print_records(char *filename, size_t size, size_t records) ;

void sort_lib(char *filename, size_t size, size_t records) ;

void read_lib(unsigned char *buff, size_t size, FILE *fp) ;

void write_lib(unsigned char *buff, size_t size, FILE *fp) ;

void print_file(FILE *fp, size_t size, size_t records) ;

void swap_records_lib(FILE *fp, record **first, record **second) ;

void shuffle_lib(char *filename, size_t size, size_t records) ;

record *create_record(unsigned char first, size_t record_num, unsigned char *ptr, size_t size) {
    record *result = malloc(sizeof(*result));
    result->first = first;
    result->recordno = record_num;
    result->ptr = (unsigned char*) strdup((char *)ptr);
    result->size = size;
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
    size_t offset = record->recordno * record->size;

    if(fseek(fp, offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(record->ptr, record->size, 1, fp);
}

void swap_records(record **first, record **second) {
    record *tmp = *first;

    *first = *second;
    *second = tmp;

    swap(&(*first)->recordno, &(*second)->recordno);
}

int main() {
    generate("test", 10, 10);
    print_records("test", 10, 10);
    shuffle_lib("test", 10, 10);
    printf("\n\n");
    print_records("test", 10, 10);
    return 0;
}

void generate(char *filename, size_t size, size_t records) {
    FILE *rand = fopen("/dev/urandom", "r");
    FILE *generated = fopen(filename, "w");
    unsigned char *buff = calloc(size, sizeof(*buff));

    for (size_t i = 0; i < records; ++i) {
        read_lib(buff, size, rand);
        write_lib(buff, size, generated);
    }
    free(buff);

    fclose(rand);
    fclose(generated);
}

void shuffle_sys(char *filename, size_t size, size_t records) {

}

void shuffle_lib(char *filename, size_t size, size_t records) {
    FILE *fp = fopen(filename, "r+");
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
            fprintf(stderr, "Error: reached end of file.");
        } else if (ferror(fp) != 0) {
            fprintf(stderr, "Error: while reading the file");
        } else {
            perror("Error");
        }
        exit(EXIT_FAILURE);
    }
}

void write_lib(unsigned char *buff, size_t size, FILE *fp) {
    if (fwrite(buff, size, 1, fp) != 1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

void swap_records_lib(FILE *fp, record **first, record **second) {
    swap_records(first, second);
    write_record_lib(fp, *first);
    write_record_lib(fp, *second);
}

void sort_sys(char *filename, size_t size, size_t records) {
    FILE *fp = fopen(filename, "r+");
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

void sort_lib(char *filename, size_t size, size_t records) {
    FILE *fp = fopen(filename, "r+");
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

void print_file(FILE *fp, size_t size, size_t records) {
    unsigned char *buff = calloc(size, sizeof(*buff));
    long offset = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    for (size_t i = 0; i < records; ++i) {
        read_lib(buff, size, fp);
        for (size_t j = 0; j < size; ++j) {
            printf("%d ", buff[j]);
        }
        printf("\n");
    };

    fseek(fp, offset, SEEK_SET);
    free(buff);
}

void print_records(char *filename, size_t size, size_t records) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error");
    }
    unsigned char *buff = calloc(size, sizeof(*buff));

    for (size_t i = 0; i < records; ++i) {
        read_lib(buff, size, fp);
        for (size_t j = 0; j < size; ++j) {
            printf("%d ", buff[j]);
        }
        printf("\n");
    };

    free(buff);
    fclose(fp);
}
