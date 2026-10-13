#include "common.h"

#include <sys/msg.h>

struct mq_msg {
  long mtype;
  unsigned char mtext[MSG_MAX];
};

static void usage(void) {
  fprintf(stderr, "использование: mq_sender <входной_файл> <размер_буфера>\n");
  fprintf(stderr, "сначала запустите mq_receiver с тем же размером буфера\n");
  exit(2);
}

static void mq_send(int qid, const void *data, size_t n) {
  struct mq_msg msg;

  if (n > MSG_MAX)
    fail("сообщение длиннее MSG_MAX");

  msg.mtype = 1;
  memcpy(msg.mtext, data, n);
  while (msgsnd(qid, &msg, n, 0) < 0) {
    if (errno == EINTR)
      continue;
    die("msgsnd");
  }
}

int main(int argc, char **argv) {
  size_t bufsize;
  uint64_t filesize;
  uint64_t left;
  int in_fd;
  int qid;
  unsigned char *buf;
  struct xfer_hdr hdr;

  if (argc != 3)
    usage();

  bufsize = parse_bufsize(argv[2]);
  in_fd = open_input(argv[1], &filesize);

  qid = msgget(make_key(PROJ_MQ), 0666);
  if (qid < 0)
    die("msgget (сначала запустите mq_receiver)");

  buf = malloc(bufsize);
  if (buf == NULL)
    die("malloc");

  /*
   * Ядро не примет сообщение больше msgmax 8192 и держит в очереди
   * всего msgmnb 16384 байт. Большой пользовательский буфер поэтому
   * режется на куски MSG_MAX, число msgsnd почти не зависит от bufsize.
   */
  hdr.filesize = filesize;
  hdr.t0_ns = monotonic_ns();
  mq_send(qid, &hdr, sizeof hdr);

  left = filesize;
  while (left > 0) {
    size_t chunk = bufsize;
    size_t off = 0;

    if ((uint64_t)chunk > left)
      chunk = (size_t)left;
    read_full(in_fd, buf, chunk);

    while (off < chunk) {
      size_t piece = chunk - off;
      if (piece > MSG_MAX)
        piece = MSG_MAX;
      mq_send(qid, buf + off, piece);
      off += piece;
    }
    left -= chunk;
  }

  free(buf);
  close(in_fd);
  return 0;
}
