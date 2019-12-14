#define _XOPEN_SOURCE 700  // for nftw & utimensat

#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

#define handle_error(msg)   \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

char to[PATH_MAX], from[PATH_MAX], next_to[PATH_MAX], rpath[PATH_MAX];
char content_buf[1024];

size_t start_index;

int mkdir_p(char path[]) {
    // 存在就直接返回执行mkdir存在的返回值
    struct stat sb;
    if (0 == stat(path, &sb)) {
        errno = EEXIST;
        return -1;
    }

    char* p = NULL;
    sprintf(rpath, "%s", path);  // 反正还不到处理符号链接的时候，借用了
    if ('/' == rpath[strlen(rpath) - 1]) rpath[strlen(rpath) - 1] = 0;
    for (p = rpath + 1; *p; p++) {
        if ('/' == *p) {
            *p = 0;
            if (-1 == mkdir(rpath, S_IRWXU) && EEXIST != errno) return -1;
            *p = '/';
        }
    }
    return mkdir(rpath, S_IRWXU);
}

int walk_action(const char* fpath, const struct stat* sb, int typeflag,
                struct FTW* ftwbuf) {
    sprintf(next_to, "%s%s", to,
            strlen(fpath) < start_index ? "" : fpath + start_index);
    struct timespec times[2];
    times[0] = sb->st_atim;
    times[1] = sb->st_mtim;
    if (typeflag == FTW_DP) {
        // 可能是个空文件夹，为了保险还得mkdir
        if (-1 == mkdir_p(next_to)) {
            if (EEXIST != errno) handle_error("mkdir failed");
        }
        if (-1 == chmod(next_to, sb->st_mode))
            handle_error("change directory mod");
        if (-1 == utimensat(AT_FDCWD, next_to, times, 0))
            handle_error("set directory time");
    } else if (FTW_F == typeflag || FTW_SL == typeflag) {
        // 创建文件夹
        size_t null_index = strlen(next_to) - strlen(fpath + ftwbuf->base);
        char temp = next_to[null_index];
        next_to[null_index] = 0;
        if (-1 == mkdir_p(next_to)) {
            if (EEXIST != errno) handle_error("mkdir failed");
        }
        next_to[null_index] = temp;

        if (FTW_F == typeflag) {
            int from_fd = open(fpath, O_RDONLY);
            if (-1 == from_fd) handle_error("open source file");
            int to_fd = open(next_to, O_CREAT | O_WRONLY, sb->st_mode);
            if (-1 == to_fd) handle_error("create target file");

            if (-1 == sendfile(to_fd, from_fd, 0, sb->st_size))
                handle_error("send file");

            close(from_fd);
            close(to_fd);
        } else {
            realpath(fpath, rpath);
            symlink(rpath, next_to);
        }
        if (-1 == utimensat(AT_FDCWD, next_to, times, AT_SYMLINK_NOFOLLOW))
            handle_error("set file time");
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) puts("Not enough arguments");

    realpath(argv[1], from);
    realpath(argv[2], to);
    strcat(from, "/");
    strcat(to, "/");

    start_index = strlen(from);

    int nftw_res = nftw(from, walk_action, 20, FTW_DEPTH | FTW_PHYS);
    if (-1 == nftw_res) {
        perror("nftw");
        exit(EXIT_FAILURE);
    } else if (0 == nftw_res)
        exit((EXIT_SUCCESS));
}