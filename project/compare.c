// === TASK 4: CHANGE DETECTION (COMPARISON ENGINE) ===
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "utils.h"
#include "compare.h"

void run_compare(char *snap_name) {
  char meta_path[512];
  int fd_meta;
  char buf[2048];
  int n;

  fmt_path(meta_path, "snapshots", snap_name);
  fmt_path(meta_path, meta_path, "snapshot.meta");

  if((fd_meta = open(meta_path, O_RDONLY)) < 0) {
    printf("snapshot compare: snapshot ledger '%s' not found\n", meta_path);
    return;
  }

  printf("Comparing live file system state against snapshot '%s':\n", snap_name);
  printf("----------------------------------------------------------\n");

  if((n = read(fd_meta, buf, sizeof(buf) - 1)) > 0) {
    buf[n] = '\0';

    char *line_start = buf;
    for(int i = 0; i <= n; i++) {
      if(buf[i] == '\n' || buf[i] == '\0') {
        if(buf[i] == '\n') buf[i] = '\0';

        char *comma = 0;
        for(char *c = line_start; *c != '\0'; c++) {
          if(*c == ',') {
            comma = c;
            break;
          }
        }

        if(comma != 0) {
          *comma = '\0';
          char *filename = line_start;
          char *size_str = comma + 1;
          int old_size = atoi(size_str);

          char live_filepath[512];
          fmt_path(live_filepath, ".", filename);

          struct stat st;
          int live_fd = open(live_filepath, O_RDONLY);

          if(live_fd < 0) {
            printf("Deleted:  %s\n", filename);
          } else {
            if(fstat(live_fd, &st) >= 0) {
              if((int)st.size != old_size) {
                printf("Modified: %s (Old size: %d Bytes | New size: %d Bytes)\n", filename, old_size, (int)st.size);
              }
            }
            close(live_fd);
          }
        }
        line_start = &buf[i + 1];
      }
    }
  }
  close(fd_meta);
}
