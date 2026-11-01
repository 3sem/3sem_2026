#include "common.h"

void die(const char *what)
{
    perror(what);
    exit(1);
}

void fail(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void ensure_keyfile(void)
{
    int fd = open(KEY_PATH, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
        die("open keyfile");
    close(fd);
}

key_t make_key(int proj)
{
    key_t key;

    ensure_keyfile();
    key = ftok(KEY_PATH, proj);
    if (key == (key_t)-1)
        die("ftok");
    return key;
}

uint64_t monotonic_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
        die("clock_gettime");
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

void write_full(int fd, const void *buf, size_t n)
{
    const unsigned char *p = buf;

    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            die("write");
        }
        if (w == 0)
            fail("write вернул 0");
        p += (size_t)w;
        n -= (size_t)w;
    }
}

void read_full(int fd, void *buf, size_t n)
{
    unsigned char *p = buf;

    while (n > 0) {
        ssize_t r = read(fd, p, n);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            die("read");
        }
        if (r == 0)
            fail("неожиданный конец данных");
        p += (size_t)r;
        n -= (size_t)r;
    }
}

size_t parse_bufsize(const char *s)
{
    char *end = NULL;
    unsigned long long v;

    errno = 0;
    v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v == 0 || v > MAX_BUF) {
        fprintf(stderr, "размер буфера: целое от 1 до %u\n", MAX_BUF);
        exit(2);
    }
    return (size_t)v;
}

int open_input(const char *path, uint64_t *size_out)
{
    int fd = open(path, O_RDONLY);
    struct stat st;

    if (fd < 0)
        die("open input");
    if (fstat(fd, &st) < 0)
        die("fstat");
    if (!S_ISREG(st.st_mode))
        fail("нужен обычный файл");
    *size_out = (uint64_t)st.st_size;
    return fd;
}

int open_output(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        die("open output");
    return fd;
}

void print_report(const char *method, size_t bufsize, uint64_t nbytes,
                  uint64_t t0_ns, uint64_t t1_ns)
{
    double sec;

    if (t1_ns < t0_ns)
        fail("монотонные часы пошли назад");
    sec = (double)(t1_ns - t0_ns) / 1e9;
    printf("method %s\nbufsize %zu\nbytes %" PRIu64 "\nseconds %.9f\n",
           method, bufsize, nbytes, sec);
    fflush(stdout);
}
