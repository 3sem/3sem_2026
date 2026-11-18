/* Данный код создаёт директорию my_dir в текущей директории, после чего созадёт символическую ссылку my_new_dir на my_dir, уничтожаемую после ввода пользователя  */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
  int res;
  res = mkdir("my_dir", S_IRUSR|S_IRGRP|S_IROTH|S_IXUSR); // вопрос: что это за флаги доступа? Каков смысл в том, чтоб ставить флаг +x?
  res = symlink("my_dir", "my_new_dir");
  char c = getchar();
  unlink("my_new_dir");
  return 0;
}
