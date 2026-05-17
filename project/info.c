// === UPGRADE: INFO VIEWING ENGINE ===
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "utils.h"
#include "info.h"

void run_info(char *snap_name) {
  static char meta_path[512];
  static char buf[2048];
  
  int fd_meta;
  int n;
  int file_count = 0;
  int total_size = 0;

  fmt_path(meta_path, "snapshots", snap_name);
  fmt_path(meta_path, meta_path, "snapshot.meta");

  if((fd_meta = open(meta_path, O_RDONLY)) < 0) {
    printf("snapshot info: Snapshot '%s' does not exist.\n", snap_name);
    return;
  }

  printf("Snapshot Information for '%s':\n", snap_name);
  printf("----------------------------------------\n");

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
          int size = atoi(comma + 1);
          
          printf(" -> %s (%d Bytes)\n", filename, size);
          
          file_count++;
          total_size += size;
        }
        line_start = &buf[i + 1];
      }
    }
  }
  
  close(fd_meta);
  printf("----------------------------------------\n");
  printf("Summary: Total Files: %d | Combined Footprint: %d Bytes\n", file_count, total_size);
}