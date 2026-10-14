#include "common.h"

#include <sys/msg.h>

struct mq_msg {
  long mtype;
  unsigned char mtext[MSG_MAX];
};

static void usage(void) {
  fprintf(stderr,
          "использование: mq_receiver <выходной_файл> <размер_буфера>\n");
  exit(2);
}

static int create_queue(key_t key) {
  int id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
  if (id >= 0)
    return id;
  if (errno != EEXIST)
    die("msgget");
  id = msgget(key, 0666);
  if (id >= 0 && msgctl(id, IPC_RMID, NULL) < 0)
    die("msgctl IPC_RMID");
  id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
  if (id < 0)
    die("msgget");
  return id;
}

static size_t mq_recv(int qid, unsigned char *dst, size_t cap) {
  struct mq_msg msg;
  ssize_t n;

  do {
    n = msgrcv(qid, &msg, MSG_MAX, 0, 0);
  } while (n < 0 && errno == EINTR);

  if (n < 0)
    die("msgrcv");
  if ((size_t)n > cap)
    fail("сообщение не влезает в буфер приёма");
  memcpy(dst, msg.mtext, (size_t)n);
  return (size_t)n;
}

int main(int argc, char **argv) {
  size_t bufsize;
  int out_fd;
  int qid;
  size_t fill;
  uint64_t got;
  uint64_t t1;
  struct xfer_hdr hdr;
  unsigned char hdrbuf[sizeof(struct xfer_hdr)];
  unsigned char *buf;

  if (argc != 3)
    usage();

  bufsize = parse_bufsize(argv[2]);
  out_fd = open_output(argv[1]);
  qid = create_queue(make_key(PROJ_MQ));

  buf = malloc(bufsize);
  if (buf == NULL)
    die("malloc");

  fprintf(stderr, "READY\n");
  fflush(stderr);

  if (mq_recv(qid, hdrbuf, sizeof hdrbuf) != sizeof hdr)
    fail("первое сообщение должно быть заголовком передачи");
  memcpy(&hdr, hdrbuf, sizeof hdr);

  /*
   * В очередь сообщение всё равно не длиннее MSG_MAX.
   * bufsize здесь это сколько байт копим перед одним write(),
   * чтобы крупный буфер уменьшал число записей в файл, но не размер сам IPC.
   */
  got = 0;
  fill = 0;
  t1 = 0;
  while (got < hdr.filesize) {
    size_t n;
    size_t space;

    /* Следующее сообщение может быть до MSG_MAX и должно влазить целиком. */
    if (fill > 0 && bufsize - fill < MSG_MAX) {
      write_full(out_fd, buf, fill);
      fill = 0;
    }
    space = bufsize - fill;
    n = mq_recv(qid, buf + fill, space);
    if (n == 0)
      fail("пустое сообщение в середине файла");
    if (got + n > hdr.filesize)
      fail("принято больше байт, чем объявлено");
    fill += n;
    got += n;

    if (fill == bufsize || got == hdr.filesize) {
      write_full(out_fd, buf, fill);
      fill = 0;
      if (got == hdr.filesize)
        t1 = monotonic_ns();
    }
  }
  if (hdr.filesize == 0)
    t1 = monotonic_ns();

  print_report("mq", bufsize, got, hdr.t0_ns, t1);

  free(buf);
  close(out_fd);
  msgctl(qid, IPC_RMID, NULL);
  return 0;
}
