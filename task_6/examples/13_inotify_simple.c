#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/inotify.h>

#define BUF (8*1024)
#define MASK (IN_ACCESS | IN_ATTRIB | IN_OPEN | IN_CLOSE_WRITE | \
              IN_CLOSE_NOWRITE | IN_CREATE | IN_DELETE | IN_MODIFY | \
              IN_MOVED_FROM | IN_MOVED_TO)

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <dir>\n", argv[0]);
    return 1;
  }

  int fd = inotify_init();
  if (fd < 0) { perror("inotify_init"); return 1; }

  int wd = inotify_add_watch(fd, argv[1], MASK);
  if (wd < 0) { perror("inotify_add_watch"); return 1; }

  printf("Watching %s...\n", argv[1]);

  char buf[BUF];
  for (;;) {
    ssize_t len = read(fd, buf, sizeof buf);
    if (len <= 0) { perror("read"); return 1; }

    for (struct inotify_event *e = (struct inotify_event *)buf;
         (char *)e < buf + len;
         e = (struct inotify_event *)((char *)e + sizeof(*e) + e->len)) {

      printf("%s: ", e->len ? e->name : ".");

      if (e->mask & IN_ACCESS)        printf("ACCESS ");
      if (e->mask & IN_ATTRIB)        printf("ATTRIB ");
      if (e->mask & IN_OPEN)          printf("OPEN ");
      if (e->mask & IN_CLOSE_WRITE)   printf("CLOSE_WRITE ");
      if (e->mask & IN_CLOSE_NOWRITE) printf("CLOSE_NOWRITE ");
      if (e->mask & IN_CREATE)        printf("CREATE ");
      if (e->mask & IN_DELETE)        printf("DELETE ");
      if (e->mask & IN_MODIFY)        printf("MODIFY ");
      if (e->mask & IN_MOVED_FROM)    printf("MOVED_FROM(%d) ", e->cookie);
      if (e->mask & IN_MOVED_TO)      printf("MOVED_TO(%d) ", e->cookie);

      printf("\n");
      fflush(stdout);
    }
  }
  
  return 0;
}
