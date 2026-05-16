// === TASK 1: FILE SYSTEM SCANNER ===
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "utils.h"
#include "scanner.h"

void run_scan(char *path) {
  char buf[512];
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, O_RDONLY)) < 0){
    printf("snapshot scan: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf("snapshot scan: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf("%s | %d Bytes\n", path, (int)st.size);
    break;

  case T_DIR:
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;

      fmt_path(buf, path, de.name);

      int child_fd = open(buf, O_RDONLY);
      if(child_fd < 0) continue;

      if(fstat(child_fd, &st) >= 0) {
        if(st.type == T_FILE) {
          printf("%s | %d Bytes\n", buf, (int)st.size);
        } else if(st.type == T_DIR) {
          close(child_fd);
          run_scan(buf);
          continue;
        }
      }
      close(child_fd);
    }
    break;
  }
  close(fd);
}
