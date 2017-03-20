#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

void print_file_info(struct stat *pStat, char *path);

void search_path(char *path, off_t file_size) ;

void search_dir(DIR *dir, char *path, off_t file_size) ;

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Error: Wrong number of arguments. Specify path and file size");
        exit(EXIT_FAILURE);
    }

    search_path(argv[1], atoi(argv[2]));
}

void search_path(char *path, off_t file_size) {
    const int MAX_PATH = 4096;
    char *rpath = malloc(MAX_PATH * sizeof(*rpath));
    realpath(path, rpath);

    DIR *dir = opendir(rpath);
    if (dir == NULL) {
        perror("Error");
    }
    search_dir(dir, path, file_size);
    free(rpath);
}

void search_dir(DIR *dir, char *path, off_t file_size) {
    if (dir == NULL) return;
    char *rpath;
    const int MAX_PATH = 4096;
    rpath = malloc(MAX_PATH * sizeof(*rpath));

    struct dirent *ent = readdir(dir);
    struct stat *st = malloc(sizeof(*st));
    while (ent != NULL) {
        realpath(ent->d_name, rpath);

        if (lstat(rpath, st) == -1) {
            perror("Error");
        }
        else if (st->st_mode == S_IFDIR && st->st_mode != S_IFLNK) {
            DIR *in_dir = opendir(rpath);
            if (in_dir == NULL) {
                perror("Error");
            } else {
                search_dir(in_dir, path, file_size);
            }
        }
        ent = readdir(dir);
    }

    rewinddir(dir);
    ent = readdir(dir);

    while (ent != NULL) {
        realpath(ent->d_name, rpath);

        if (lstat(rpath, st) != 0 && st->st_mode == S_IFREG && st->st_size <= file_size) {
            print_file_info(st, rpath);
        }
        ent = readdir(dir);
    }

    free(rpath);
    free(st);
}

void print_file_info(struct stat *pStat, char *path) {
    char access[12];
    char mod[12];

    printf("Path: %s\nSize: %ld\nAccess: %d\nModification: %s\n\n", path, pStat->st_size, pStat->st_mode,
           ctime((const time_t *) &pStat->st_mtim));
}

void search_dir_nftw(char *path) {

}