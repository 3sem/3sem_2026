#include "echo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void usage(const char *argv0) {
  fprintf(stderr,
          "использование: %s [--copy] <входной_файл> <выходной_файл>\n"
          "  без флага  splice, байты не заходят в буфер программы\n"
          "  --copy     read/write через свой буфер\n",
          argv0);
}

int main(int argc, char **argv) {
  EchoMode mode = ECHO_MODE_SPLICE;
  int arg = 1;
  pid_t child_pid = -1;
  DuplexPipe *dp;
  int status = 0;
  int rc;

  if (argc == 4 && strcmp(argv[1], "--copy") == 0) {
    mode = ECHO_MODE_COPY;
    arg = 2;
  } else if (argc != 3) {
    usage(argv[0]);
    return 2;
  }

  dp = constructDuplexPipe(&child_pid);
  if (dp == NULL) {
    perror("constructDuplexPipe");
    return 1;
  }

  if (child_pid == 0) {
    int child_rc = echo_child(dp, mode);
    dp->actions.destroy(dp);
    _exit(child_rc == 0 ? 0 : 1);
  }

  rc = echo_parent(dp, argv[arg], argv[arg + 1], mode);

  dp->actions.close_write(dp);
  if (waitpid(child_pid, &status, 0) < 0) {
    perror("waitpid");
    dp->actions.destroy(dp);
    return 1;
  }
  dp->actions.destroy(dp);

  if (WIFSIGNALED(status)) {
    fprintf(stderr, "child killed by signal %d\n", WTERMSIG(status));
    return 1;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "child failed\n");
    return 1;
  }
  return rc == 0 ? 0 : 1;
}
