#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <memory.h>
#include <errno.h>

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
        fprintf(stderr, "Error: %s %s\n", rpath, strerror(errno));
    }
    search_dir(dir, rpath, file_size);
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
        strncpy(rpath, path, MAX_PATH);
        strcat(rpath, "/");
        strncat(rpath, ent->d_name, MAX_PATH);

        if (lstat(rpath, st) == -1) {
            fprintf(stderr, "Error: %s %s\n", rpath, strerror(errno));
        }
        else if (strcmp(ent->d_name, "..") != 0 && strcmp(ent->d_name, ".") != 0
                 && S_ISDIR(st->st_mode) && S_ISLNK(st->st_mode) == 0) {
            DIR *in_dir = opendir(rpath);
            if (in_dir == NULL) {
                fprintf(stderr, "Error: %s %s\n", rpath, strerror(errno));
            } else {
                search_dir(in_dir, rpath, file_size);
            }
        }
        ent = readdir(dir);
    }

    rewinddir(dir);
    ent = readdir(dir);

    while (ent != NULL) {
        strncpy(rpath, path, MAX_PATH);
        strcat(rpath, "/");
        strncat(rpath, ent->d_name, MAX_PATH);

        if (lstat(rpath, st) != -1 && S_ISREG(st->st_mode) && st->st_size <= file_size) {
            print_file_info(st, rpath);
        }
        ent = readdir(dir);
    }

    free(rpath);
    free(st);
}

void print_file_info(struct stat *pStat, char *path) {
    char access[9];

    access[0] = (char) (pStat->st_mode & S_IRUSR ? 'r' : '-');
    access[1] = (char) (pStat->st_mode & S_IWUSR ? 'w' : '-');
    access[2] = (char) (pStat->st_mode & S_IXUSR ? 'x' : '-');
    access[3] = (char) (pStat->st_mode & S_IRGRP ? 'r' : '-');
    access[4] = (char) (pStat->st_mode & S_IWGRP ? 'w' : '-');
    access[5] = (char) (pStat->st_mode & S_IXGRP ? 'x' : '-');
    access[6] = (char) (pStat->st_mode & S_IROTH ? 'r' : '-');
    access[7] = (char) (pStat->st_mode & S_IWOTH ? 'w' : '-');
    access[8] = (char) (pStat->st_mode & S_IXOTH ? 'x' : '-');

    printf("Path: %s\nSize: %ld Bytes\nPermissions: %s\nModification: %s\n\n", path, pStat->st_size, access,
           ctime((const time_t *) &pStat->st_mtim));
}

void search_dir_nftw(char *path) {

}