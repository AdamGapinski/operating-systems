#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>


void print_menu();

long long int chose_char_num();

void set_read_lock(char *filename) ;

void set_write_lock(char *filename) ;

void read_char(char *filename) ;

void write_char(char *filename) ;

void lock_info(char *filename) ;

void release_lock(char *filename) ;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arguments. Specify filename.\n");
        exit(EXIT_FAILURE);
    }
    char *filename = argv[1];
    if (access(filename, F_OK) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    char *buff = calloc(20, sizeof(*buff));
    int exit = 0;

    while (exit == 0) {
        print_menu();
        scanf("%s", buff);
        if (strcmp(buff, "7") == 0) {
            exit = 1;
        } else if (strcmp(buff, "1") == 0) {
            set_read_lock(filename);
        } else if (strcmp(buff, "2") == 0) {
            set_write_lock(filename);
        } else if (strcmp(buff, "3") == 0) {
            lock_info(filename);
        } else if (strcmp(buff, "4") == 0) {
            release_lock(filename);
        } else if (strcmp(buff, "5") == 0) {
            read_char(filename);
        } else if (strcmp(buff, "6") == 0) {
            write_char(filename);
        } else {
            fprintf(stderr, "Unrecognized option; %s\n", buff);
        }
    }

    free(buff);
}

void print_menu() {
    printf("Wybierz operacje (1 - 7)\n");
    printf("1. ustawienie rygla do odczytu na wybrany znak pliku,\n");
    printf("2. ustawienie rygla do zapisu na wybrany znak pliku,\n");
    printf("3. wyswietlenie listy zaryglowanych znakow pliku (z informacja o numerze PID procesu będacego wlascicielem rygla oraz jego typie - odczyt/zapis),\n");
    printf("4. zwolnienie wybranego rygla,\n");
    printf("5. odczyt (funkcją read) wybranego znaku pliku,\n");
    printf("6. zmiana (funkcją write) wybranego znaku pliku.\n");
    printf("7. wyjscie\n");
}

int open_file(char *filename) ;

char chose_char();

void write_char(char *filename) {
    int fd = open(filename, O_RDWR);

    long long int char_n = chose_char_num();
    lseek(fd, char_n, SEEK_SET);

    char *to_write = calloc(1, sizeof(to_write));
    to_write[0] = chose_char();

    if (write(fd, to_write, 1) == -1) {
        perror("Error, while writing char");
    }

    if (close(fd) == -1) {
        perror("Error");
    }

    free(to_write);
}

char chose_char() {
    char *buff = calloc(5, sizeof(*buff));
    char result = 0;

    printf("podaje znak: \n");
    scanf("%s", buff);
    sscanf(buff, "%c", &result);

    return result;
}

void read_char(char *filename) {
    int fd = open_file(filename);

    long long int char_n = chose_char_num();
    lseek(fd, char_n, SEEK_SET);

    char *to_read = calloc(1, sizeof(to_read));

    if (read(fd, to_read, 1) == -1) {
        perror("Error, while reading char");
    }

    if (close(fd) == -1) {
        perror("Error");
    }

    printf("Odczytany znak to %s\n", to_read);
    free(to_read);
}

void release_lock(char *filename) {
    /*int fd = open_file(filename);

    if (close(fd) == -1) {
        perror("Error");
    }*/
}

void lock_info(char *filename) {
    int fd = open_file(filename);


    if (close(fd) == -1) {
        perror("Error");
    }
}

int chose_version() {
    char *buff = calloc(20, sizeof(*buff));
    int result = -1;

    printf("wybierz wersje (1 - 3)\n");
    printf("1. wersja nieblokujaca\n");
    printf("2. wersja blokujaca\n");
    printf("3. powrot\n");

    scanf("%s", buff);

    if (strcmp(buff, "1") == 0) {
        result = 1;
    } else if (strcmp(buff, "2") == 0) {
        result = 2;
    } else if (strcmp(buff, "3") == 0) {
        result = 3;
    } else {
        result = -1;
        fprintf(stderr, "Error: unrecognized option\n");
    }

    free(buff);
    return result;
}

int open_file(char *filename) ;

void make_lock_nowait(int fd, struct flock *fl) ;

void make_lock_wait(int fd, struct flock *fl) ;

struct flock* flock_write(long long l_char) ;

void set_write_lock(char *filename) {
    int version = chose_version();
    if (version == -1 || version == 3) {
        return;
    }
    long long char_n = chose_char_num();
    struct flock *fl = flock_write(char_n);
    int fd = open_file(filename);

    if (version == 0) {
        make_lock_nowait(fd, fl);
    } else if (version == 1) {
        make_lock_wait(fd, fl);
    }

    if (close(fd) == -1) {
        perror("Error");
    }
    free(fl);
}

int open_file(char *filename) ;

void make_lock_nowait(int fd, struct flock *fl) ;

void make_lock_wait(int fd, struct flock *fl) ;

struct flock* flock_read(long long l_char) ;

void set_read_lock(char *filename) {
    int version = chose_version();
    if (version == -1 || version == 3) {
        return;
    }
    long long char_n = chose_char_num();
    struct flock *fl = flock_read(char_n);
    int fd = open_file(filename);

    if (version == 0) {
        make_lock_nowait(fd, fl);
    } else if (version == 1) {
        make_lock_wait(fd, fl);
    }
    free(fl);

    if (close(fd) == -1) {
        perror("Error");
    }
}

long long chose_char_num() {
    char *buff = calloc(20, sizeof(*buff));
    long long result = 0;

    printf("podaje numer znaku: \n");
    scanf("%s", buff);
    sscanf(buff, "%lld", &result);

    return result;
}

int open_file(char *filename) {
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    return fd;
}

void make_lock_wait(int fd, struct flock *fl) {
    if (fcntl(fd, F_SETLKW, fl) == -1) {
        perror("Error");
    }
}

void make_lock_nowait(int fd, struct flock *fl) {
    if (fcntl(fd, F_SETLK, fl) == -1) {
        perror("Error");
    }
}

struct flock* flock_write(long long l_char) {
    struct flock *fl = malloc(sizeof(*fl));
    fl->l_type = F_WRLCK;
    fl->l_whence = SEEK_SET;
    fl->l_start = l_char;
    fl->l_len = 1;
    return fl;
}

struct flock* flock_read(long long l_char) {
    struct flock *fl = malloc(sizeof(*fl));
    fl->l_type = F_RDLCK;
    fl->l_whence = SEEK_SET;
    fl->l_start = l_char;
    fl->l_len = 1;
    return fl;

}
