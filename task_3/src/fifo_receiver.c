#include "common.h"

static void usage(void)
{
    fprintf(stderr, "использование: fifo_receiver <выходной_файл> <размер_буфера>\n");
    exit(2);
}

int main(int argc, char **argv)
{
    size_t bufsize;
    int out_fd;
    int fifo_fd;
    uint64_t got;
    uint64_t t1;
    struct xfer_hdr hdr;
    unsigned char *buf;

    if (argc != 3)
        usage();

    bufsize = parse_bufsize(argv[2]);
    out_fd = open_output(argv[1]);

    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0666) < 0)
        die("mkfifo");

    fprintf(stderr, "READY\n");
    fflush(stderr);

    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0)
        die("open fifo");

    buf = malloc(bufsize);
    if (buf == NULL)
        die("malloc");

    read_full(fifo_fd, &hdr, sizeof hdr);

    got = 0;
    while (got < hdr.filesize) {
        size_t want = bufsize;
        ssize_t r;

        if ((uint64_t)want > hdr.filesize - got)
            want = (size_t)(hdr.filesize - got);

        do {
            r = read(fifo_fd, buf, want);
        } while (r < 0 && errno == EINTR);

        if (r < 0)
            die("read fifo");
        if (r == 0)
            fail("канал закрыт раньше конца файла");

        write_full(out_fd, buf, (size_t)r);
        got += (size_t)r;
    }
    t1 = monotonic_ns();

    print_report("fifo", bufsize, got, hdr.t0_ns, t1);

    free(buf);
    close(fifo_fd);
    close(out_fd);
    unlink(FIFO_PATH);
    return 0;
}
