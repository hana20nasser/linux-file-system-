// === TASK 2: FILE COPY ENGINE ===
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "copy.h"

void copy_file(char *src, char *dest) {
  int fd_src, fd_dest;
  int n;
  char buf[512];

  if((fd_src = open(src, O_RDONLY)) < 0) return;

  if((fd_dest = open(dest, O_WRONLY | O_CREATE | O_TRUNC)) < 0) {
    close(fd_src);
    return;
  }

  while((n = read(fd_src, buf, sizeof(buf))) > 0) {
    write(fd_dest, buf, n);
  }

  close(fd_src);
  close(fd_dest);
}
