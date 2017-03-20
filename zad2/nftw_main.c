#define _XOPEN_SOURCE 500
#include <time.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>

void print_file_info(const struct stat *pStat, const char *path) ;

void search_dir(char *path) ;

off_t upper_bound_size;

int main(int argc, char **argv) {

    if (argc != 3) {
        fprintf(stderr, "Error: Wrong number of arguments. Specify path and file size\n");
        exit(EXIT_FAILURE);
    }

    upper_bound_size = atoi(argv[2]);

    search_dir(argv[1]);

    return 0;
}

int process_dir(const char *fpath, const struct stat *sb, int tflag,
                struct FTW *ftwbuf) { {}
    if (tflag == FTW_F && sb->st_size <= upper_bound_size) {
        print_file_info(sb, fpath);
    }
    return 0;
}

void search_dir(char *path) {
    int flags = 0;
    flags |= FTW_DEPTH;
    flags |= FTW_PHYS;

    if (nftw(path, process_dir, 20, flags) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

void print_file_info(const struct stat *pStat, const char *path) {
    char access[10];

    access[0] = (char) (pStat->st_mode & S_IRUSR ? 'r' : '-');
    access[1] = (char) (pStat->st_mode & S_IWUSR ? 'w' : '-');
    access[2] = (char) (pStat->st_mode & S_IXUSR ? 'x' : '-');
    access[3] = (char) (pStat->st_mode & S_IRGRP ? 'r' : '-');
    access[4] = (char) (pStat->st_mode & S_IWGRP ? 'w' : '-');
    access[5] = (char) (pStat->st_mode & S_IXGRP ? 'x' : '-');
    access[6] = (char) (pStat->st_mode & S_IROTH ? 'r' : '-');
    access[7] = (char) (pStat->st_mode & S_IWOTH ? 'w' : '-');
    access[8] = (char) (pStat->st_mode & S_IXOTH ? 'x' : '-');
    access[9] = '\0';

    printf("Path: %s\nSize: %ld Bytes\nPermissions: %s\nModification: %s\n\n", path, pStat->st_size, access,
           ctime((const time_t *) &pStat->st_mtime));
}