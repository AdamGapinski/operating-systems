#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>
#include <sys/stat.h>


void print_menu();

void set_read_lock(int fd) ;

void set_write_lock(int fd) ;

void read_char(int fd) ;

void write_char(int fd) ;

void lock_info(int fd) ;

void release_lock(int fd) ;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arguments. Specify filename.\n");
        exit(EXIT_FAILURE);
    }
    char *filename = argv[1];
    int fd;
    if ((fd = open(filename, O_RDWR)) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    char *buff = calloc(20, sizeof(*buff));

    while (1) {
        print_menu();
        scanf("%s", buff);
        if (strcmp(buff, "1") == 0) {
            set_read_lock(fd);
        } else if (strcmp(buff, "2") == 0) {
            set_write_lock(fd);
        } else if (strcmp(buff, "3") == 0) {
            lock_info(fd);
        } else if (strcmp(buff, "4") == 0) {
            release_lock(fd);
        } else if (strcmp(buff, "5") == 0) {
            read_char(fd);
        } else if (strcmp(buff, "6") == 0) {
            write_char(fd);
        } else if (strcmp(buff, "7") == 0) {
            break;
        } else {
            fprintf(stderr, "Unrecognized option: %s\n", buff);
        }
    }

    close(fd);
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

char chose_char();

long chose_char_num() ;

void write_char(int fd) {
    long long int char_n = chose_char_num();
    lseek(fd, char_n, SEEK_SET);

    char *to_write = calloc(1, sizeof(to_write));
    to_write[0] = chose_char();

    if (write(fd, to_write, 1) == -1) {
        perror("Error, while writing char");
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

void read_char(int fd) {
    long long int char_n = chose_char_num();
    lseek(fd, char_n, SEEK_SET);

    char *to_read = calloc(1, sizeof(to_read));

    if (read(fd, to_read, 1) == -1) {
        perror("Error, while reading char");
    }

    printf("Odczytany znak to %s\n", to_read);
    free(to_read);
}

void release_lock(int fd) {
    /*chose_char_num()*/
}

struct flock* flock_write(long l_char) ;

struct flock* flock_read(long l_char) ;

void print_lock_info(struct flock *lock, int fd);

void lock_info(int fd) {
    struct flock *write;
    struct flock *read;
    struct stat st;
    fstat(fd, &st);
    long size = st.st_size;

    printf("%s\t%s\t%s\t%s\n", "PID", "char_num", "char", "type");

    for (int i = 0; i < size; ++i) {
        write = flock_write(i);
        read = flock_read(i);

        if ((fcntl(fd, F_GETLK, write) == -1) | (fcntl(fd, F_GETLK, read) == -1)) {
            perror("Error while getting flock");
            exit(EXIT_FAILURE);
        }

        if (write->l_type != F_UNLCK) {
            print_lock_info(write, fd);
        } else if (read->l_type != F_UNLCK) {
            print_lock_info(read, fd);
        }

        free(write);
        free(read);
    }
}

void print_lock_info(struct flock *lock, int fd) {
    off_t prev_offset = lseek(fd, 0, SEEK_SET);
    lseek(fd, lock->l_start, SEEK_SET);

    char *locked = calloc(2, sizeof(*locked));
    if(read(fd, locked, 1) == -1) {
        perror("Error while printing lock info.");
    };
    if (strcmp(locked, "\00") == 0) {
        strcpy(locked, " ");
    }
    char *type = lock->l_type == F_RDLCK ? "odczyt" : lock->l_type == F_WRLCK ? "zapis" : "niezdefiniowany";
    printf("%d\t%ld\t%s\t%s\n", lock->l_pid, lock->l_start, locked, type);
    free(locked);
    lseek(fd, prev_offset, SEEK_SET);
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

void make_lock_nowait(int fd, struct flock *fl) ;

void make_lock_wait(int fd, struct flock *fl) ;

struct flock* flock_write(long l_char) ;

void set_write_lock(int fd) {
    int version = chose_version();
    if (version == -1 || version == 3) {
        return;
    }
    long long char_n = chose_char_num();
    struct flock *fl = flock_write(char_n);

    if (version == 1) {
        make_lock_nowait(fd, fl);
    } else if (version == 2) {
        make_lock_wait(fd, fl);
    }

    free(fl);
}

void make_lock_nowait(int fd, struct flock *fl) ;

void make_lock_wait(int fd, struct flock *fl) ;

long chose_char_num() ;

struct flock* flock_read(long l_char) ;

void set_read_lock(int fd) {
    int version = chose_version();
    if (version == -1 || version == 3) {
        return;
    }
    long char_n = chose_char_num();
    struct flock *fl = flock_read(char_n);

    if (version == 1) {
        make_lock_nowait(fd, fl);
    } else if (version == 2) {
        make_lock_wait(fd, fl);
    }
    free(fl);
}

long chose_char_num() {
    char *buff = calloc(20, sizeof(*buff));
    long result = 0;

    printf("podaje numer znaku: \n");
    scanf("%s", buff);
    sscanf(buff, "%ld", &result);

    return result;
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

struct flock* flock_write(long l_char) {
    struct flock *fl = malloc(sizeof(*fl));
    fl->l_type = F_WRLCK;
    fl->l_whence = SEEK_SET;
    fl->l_start = l_char;
    fl->l_len = 1;
    return fl;
}

struct flock* flock_read(long l_char) {
    struct flock *fl = malloc(sizeof(*fl));
    fl->l_type = F_RDLCK;
    fl->l_whence = SEEK_SET;
    fl->l_start = l_char;
    fl->l_len = 1;
    return fl;

}
