#define _XOPEN_SOURCE 700 // for nftw & utimensat

#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define handle_error(msg)   \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

char to[PATH_MAX], from[PATH_MAX], next_to[PATH_MAX];
char content_buf[1024];

size_t start_index;

int walk_action(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    // 文件名 fpath + ftwbuf->base
    // 文件路径 fpath
    // 文件大小 (intmax_t) sb->st_size
    sprintf(next_to, "%s%s", to, strlen(fpath) < start_index ? "" : fpath + start_index);
    printf("current: %s\n", fpath);
    struct timespec times[2];
    times[0] = sb->st_atim;
    times[1] = sb->st_mtim;
    if (typeflag == FTW_D) {
        printf("%s is a folder\n", fpath);
        if (-1 == mkdir(next_to, sb->st_mode)) {
            if (EEXIST == errno) printf("%s exists\n", fpath + ftwbuf->base);
            else handle_error("mkdir failed");
        }
        if (-1 == utimensat(AT_FDCWD, next_to, times, 0)) handle_error("set direcory time");
    } else if (typeflag == FTW_F) {
        printf("copy %s to %s\n", fpath, next_to);
        int from_fd = open(fpath, O_RDONLY);
        if (-1 == from_fd) handle_error("open source file");

        // 创建文件夹
//        size_t null_index = strlen(next_to) - strlen(fpath+ftwbuf->base);
//        char temp = next_to[null_index];
//        next_to[null_index] = 0;
//        if (-1 == mkdir(next_to, sb->st_mode)) {
//            if (EEXIST == errno) printf("%s exists\n", fpath + ftwbuf->base);
//            else handle_error("mkdir failed");
//        }
//        next_to[null_index] = temp;

        int to_fd = open(next_to, O_CREAT | O_WRONLY, sb->st_mode);
        if (-1 == to_fd) handle_error("create target file");

        int byte_read, byte_write;
        while (0 < (byte_read = read(from_fd, content_buf, sizeof content_buf))) {
            while ((byte_read = write(to_fd, content_buf, sizeof content_buf), byte_write > 0)) {
                byte_write -= byte_read;
            }
            if (-1 == byte_write) handle_error("write failed");
        }
        if (-1 == byte_read) handle_error("read failed");
        if (-1 == utimensat(AT_FDCWD, next_to, times, 0)) handle_error("set file time");

        close(from_fd);
        close(to_fd);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) puts("Not enough arguments");

    strcpy(from, argv[1]);
    strcpy(to, argv[2]);

    if ('/' != from[strlen(from) - 1]) strcat(from, "/");
    if ('/' != to[strlen(to) - 1]) strcat(to, "/");
    start_index = strlen(from);

    int nftw_res = nftw(from, walk_action, 20, 0);//FTW_DEPTH);
    if (-1 == nftw_res) {
        perror("nftw");
        exit(EXIT_FAILURE);
    } else if (0 == nftw_res)
        exit((EXIT_SUCCESS));
}