#include "common.h"

#include <sys/sem.h>
#include <sys/shm.h>

#define SEM_EMPTY 0
#define SEM_FULL 1

static void usage(void) {
  fprintf(stderr, "использование: shm_sender <входной_файл> <размер_буфера>\n");
  fprintf(stderr, "сначала запустите shm_receiver с тем же размером буфера\n");
  exit(2);
}

static void sem_op(int id, unsigned short num, short op) {
  struct sembuf sb;

  sb.sem_num = num;
  sb.sem_op = op;
  sb.sem_flg = 0;
  while (semop(id, &sb, 1) < 0) {
    if (errno == EINTR)
      continue;
    die("semop");
  }
}

int main(int argc, char **argv) {
  size_t bufsize;
  size_t region;
  uint64_t filesize;
  uint64_t left;
  uint64_t t0;
  int in_fd;
  int shmid;
  int semid;
  int first;
  unsigned char *buf;
  struct shm_slot *slot;
  struct shmid_ds ds;

  if (argc != 3)
    usage();

  bufsize = parse_bufsize(argv[2]);
  in_fd = open_input(argv[1], &filesize);
  region = sizeof(struct shm_slot) + bufsize;

  shmid = shmget(make_key(PROJ_SHM), region, 0666);
  if (shmid < 0)
    die("shmget (сначала запустите shm_receiver)");
  if (shmctl(shmid, IPC_STAT, &ds) < 0)
    die("shmctl IPC_STAT");
  if (ds.shm_segsz < region)
    fail("сегмент меньше запрошенного буфера: у обеих программ один bufsize");

  slot = shmat(shmid, NULL, 0);
  if (slot == (void *)-1)
    die("shmat");

  semid = semget(make_key(PROJ_SEM), 2, 0666);
  if (semid < 0)
    die("semget");

  buf = malloc(bufsize);
  if (buf == NULL)
    die("malloc");

  /*
   * Отметка ставится после соединения и до чтения файла.
   * Получатель останавливает часы после записи последнего байта.
   */
  t0 = monotonic_ns();
  left = filesize;
  first = 1;

  if (left == 0) {
    sem_op(semid, SEM_EMPTY, -1);
    slot->filesize = 0;
    slot->t0_ns = t0;
    slot->len = 0;
    slot->pad = 0;
    sem_op(semid, SEM_FULL, 1);
  }

  while (left > 0) {
    size_t chunk = bufsize;
    if ((uint64_t)chunk > left)
      chunk = (size_t)left;

    read_full(in_fd, buf, chunk);
    sem_op(semid, SEM_EMPTY, -1);
    if (first) {
      slot->filesize = filesize;
      slot->t0_ns = t0;
      first = 0;
    }
    slot->len = (uint32_t)chunk;
    slot->pad = 0;
    memcpy(slot->data, buf, chunk);
    sem_op(semid, SEM_FULL, 1);
    left -= chunk;
  }

  if (shmdt(slot) < 0)
    die("shmdt");
  free(buf);
  close(in_fd);
  return 0;
}
