#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include <errno.h>

static void SearchDirectory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) { perror(path); return; }

    char fullpath[PATH_MAX];
    struct stat info;
    struct dirent *e;

    errno = 0;
    while ((e = readdir(dir))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;

        int n = snprintf(fullpath, sizeof(fullpath), "%s/%s", path, e->d_name);
        if (n < 0 || (size_t)n >= sizeof(fullpath)) {
            fprintf(stderr, "path too long: %s/%s\n", path, e->d_name);
            continue;
        }

        if (lstat(fullpath, &info)) {
            perror(fullpath);
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            printf("directory: %s/\n", fullpath);
            SearchDirectory(fullpath);
        } else if (S_ISLNK(info.st_mode)) {
            printf("symlink:   %s\n", fullpath);
        } else {
            printf("file:      %s\n", fullpath);
        }
        errno = 0;
    }
    if (errno) perror("readdir");
    closedir(dir);
}

int main() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) SearchDirectory(cwd);
    return 0;
}
