#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include "include/Utils.h"

typedef struct Record {
    size_t record_num;
    size_t size;
    char *ptr;
} Record;

void read_file(char *buff, size_t size, FILE *fp) ;

void write_file(char *buff, size_t size, FILE *fp) ;

int find_text(char *text, char *key) ;

void make_unsigned(char *buff, size_t size) ;

void generate_file_with_key(char *pathname, char *key, size_t record_size,
                            size_t records, size_t des_record, int offset) ;

void prepend_index(char *buff, int index);

void print_bytes(char *buff, size_t size) ;

int main() {
    const size_t record_size = 1024;
    const size_t records = 10000;
    const size_t des_record = 5540;
    const int offset = 534;
    char *pathname = "testfile";
    char *key = "aaa";

    generate_file_with_key(pathname, key, record_size, records, des_record, offset);

    /*FILE *generated = fopen(pathname, "r");
    printf("Generated file:\n");
    char buff[record_size];
    for (int i = 0; i < records; ++i) {
        read_file(buff, record_size, generated);
        print_bytes(buff, record_size);
        printf("\n\n\n\n");
    }
    fclose(generated);*/
    return 0;
}

void write_record(FILE *fp, Record *record) {
    long prev_offset = ftell(fp);
    size_t offset = record->record_num * record->size;

    if(fseek(fp, offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    write_file(record->ptr, record->size, fp);

    if(fseek(fp, prev_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void generate_file_with_key(char *pathname, char *key, size_t record_size,
                            size_t records, size_t des_record, int offset) {
    FILE *rand_source = fopen("/dev/urandom", "r");
    FILE *generated = fopen(pathname, "w+");
    char *buff = calloc(record_size, sizeof(*buff));

    for (size_t i = 0; i < records; ++i) {
        read_file(buff, record_size, rand_source);
        make_unsigned(buff, record_size);
        prepend_index(buff, (int) i);
        if (find_text(buff + 4, key) == 0) {
            write_file(buff, record_size, generated);
        } else {
            //repeat the loop
            --i;
        }
    }

    for (int i = 0; i < strlen(key); ++i) {
        buff[i + offset] = key[i];
    }
    prepend_index(buff, (int) des_record);

    Record to_write;
    to_write.ptr = buff;
    to_write.size = record_size;
    to_write.record_num = des_record;
    write_record(generated, &to_write);

    free(buff);
    fclose(rand_source);
    fclose(generated);
}

void prepend_index(char *buff, int index) {
    buff[0] = (char) index % 256;
    buff[1] = (char) (index >> 8) % 256;
    buff[2] = (char) (index >> 16) % 256;
    buff[3] = (char) (index >> 24) % 256;
}

void make_unsigned(char *buff, size_t size) {
    for (int i = 0; i < size; ++i) {
        if (buff[i] == -128) {
            buff[i] = 127;
        } else {
            buff[i] = (char) abs(buff[i]);
        }
    }
}

int find_text(char *text, char *key) {
    size_t key_len = strlen(key);
    int record_len = RECORD_SIZE - sizeof(int);
    int key_index;
    for (int i = 0; i < record_len; ++i) {
        for (key_index = 0; key_index < key_len && i + key_index < record_len; ++key_index) {
            if (text[i + key_index] != key[key_index]) {
                break;
            }
        }
        if (key_index == key_len) {
            return 1;
        }
    }
    return 0;
}

void read_file(char *buff, size_t size, FILE *fp) {
    if (fread(buff, sizeof(*buff), size, fp) != size) {
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

void write_file(char *buff, size_t size, FILE *fp) {
    if (fwrite(buff, sizeof(*buff), size, fp) != size) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

void print_bytes(char *buff, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (buff[i] == 0) {
            printf("NULL ");
        } else {
            printf("%d ", buff[i]);
        }
    }
}
