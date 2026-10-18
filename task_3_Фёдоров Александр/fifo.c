#include "common.h"
#include <sys/stat.h>

static char directory[] = "/tmp/task3-XXXXXX";
static char path[128];
static int created = 0;

static void cleanup(void)
{
    if (created) {
        unlink(path);
        rmdir(directory);
        created = 0;
    }
}

int main(int argc, char **argv)
{
    prepare(argc, argv);
    if (mkdtemp(directory) == NULL)
        fail("mkdtemp");
    created = 1;
    snprintf(path, sizeof(path), "%s/data", directory);
    if (mkfifo(path, 0600) == -1)
        fail("mkfifo");
    int reader = open(path, O_RDONLY | O_NONBLOCK);
    if (reader == -1)
        fail("open FIFO reader");
    int writer = open(path, O_WRONLY | O_NONBLOCK);
    if (writer == -1)
        fail("open FIFO writer");
    if (fcntl(reader, F_SETFL, 0) == -1 || fcntl(writer, F_SETFL, 0) == -1)
        fail("fcntl");
    unsigned char *buffer = malloc(block_size);
    if (buffer == NULL)
        fail("malloc");

    double start = now();
    child = fork();
    if (child == -1)
        fail("fork");
    if (child == 0) {
        close(input);
        close(writer);
        ssize_t n;
        while ((n = read_block(reader, buffer, block_size)) > 0)
            write_all(output, buffer, (size_t)n);
        close(reader);
        if (close(output) == -1)
            fail("close");
        _exit(0);
    }
    close(output);
    close(reader);
    ssize_t n;
    while ((n = read_block(input, buffer, block_size)) > 0)
        write_all(writer, buffer, (size_t)n);
    close(writer); /* После этого получатель увидит EOF. */
    close(input);
    finish(start);
    free(buffer);
    return 0;
}
