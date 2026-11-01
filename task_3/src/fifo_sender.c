#include "common.h"

static void usage(void)
{
    fprintf(stderr, "использование: fifo_sender <входной_файл> <размер_буфера>\n");
    fprintf(stderr, "сначала запустите fifo_receiver с тем же размером буфера\n");
    exit(2);
}

int main(int argc, char **argv)
{
    size_t bufsize;
    uint64_t filesize;
    uint64_t left;
    int in_fd;
    int fifo_fd;
    int pipe_cap;
    unsigned char *buf;
    struct xfer_hdr hdr;

    if (argc != 3)
        usage();

    bufsize = parse_bufsize(argv[2]);
    in_fd = open_input(argv[1], &filesize);

    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0)
        die("open fifo (сначала запустите fifo_receiver)");

    pipe_cap = fcntl(fifo_fd, F_GETPIPE_SZ);
    if (pipe_cap >= 0) {
        fprintf(stderr, "PIPE_CAP %d\n", pipe_cap);
        fflush(stderr);
    }

    buf = malloc(bufsize);
    if (buf == NULL)
        die("malloc");

    hdr.filesize = filesize;
    hdr.t0_ns = monotonic_ns();
    write_full(fifo_fd, &hdr, sizeof hdr);

    left = filesize;
    while (left > 0) {
        size_t chunk = bufsize;
        if ((uint64_t)chunk > left)
            chunk = (size_t)left;
        read_full(in_fd, buf, chunk);
        write_full(fifo_fd, buf, chunk);
        left -= chunk;
    }

    free(buf);
    close(fifo_fd);
    close(in_fd);
    return 0;
}
