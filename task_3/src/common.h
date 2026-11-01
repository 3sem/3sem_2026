#ifndef COMMON_H
#define COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define KEY_PATH "/tmp/ipc_lab_task3.key"
#define FIFO_PATH "/tmp/ipc_lab_task3.fifo"

/* Потолок одного сообщения SysV на этой машине (kernel.msgmax). */
#define MSG_MAX 8192u

/* Защита от случайного огромного сегмента. Для замеров хватает 1 МиБ. */
#define MAX_BUF (16u * 1024u * 1024u)

#define PROJ_SHM 0x53
#define PROJ_SEM 0x45
#define PROJ_MQ 0x51

struct xfer_hdr {
    uint64_t filesize;
    uint64_t t0_ns;
};

/*
 * Один слот разделяемой памяти.
 * Писатель публикует кусок, читатель забирает его целиком.
 * semop даёт барьер: поля слота нельзя читать до захвата семафора.
 */
struct shm_slot {
    uint64_t filesize;
    uint64_t t0_ns;
    uint32_t len;
    uint32_t pad;
    unsigned char data[];
};

void die(const char *what);
void fail(const char *msg);
void ensure_keyfile(void);
key_t make_key(int proj);
uint64_t monotonic_ns(void);
void write_full(int fd, const void *buf, size_t n);
void read_full(int fd, void *buf, size_t n);
size_t parse_bufsize(const char *s);
int open_input(const char *path, uint64_t *size_out);
int open_output(const char *path);
void print_report(const char *method, size_t bufsize, uint64_t nbytes,
                  uint64_t t0_ns, uint64_t t1_ns);

#endif
