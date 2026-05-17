// === UPGRADE: CASCADING DELETION ENGINE ===
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "utils.h"
#include "delete.h"

void run_delete(char *snap_name) {
  static char meta_path[512];
  static char snap_dir[256];
  static char buf[2048];
  static char backup_filepath[512];

  int fd_meta;
  int n;

  fmt_path(snap_dir, "snapshots", snap_name);
  fmt_path(meta_path, snap_dir, "snapshot.meta");

  if((fd_meta = open(meta_path, O_RDONLY)) < 0) {
    printf("snapshot delete: Snapshot '%s' does not exist.\n", snap_name);
    return;
  }

  printf("Purging files inside snapshot '%s'...\n", snap_name);

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
          char *live_filepath = line_start;
          
          // Isolate raw filename base pointer
          char *filename = live_filepath;
          for(char *c = live_filepath; *c != '\0'; c++) {
            if(*c == '/') {
              filename = c + 1;
            }
          }

          fmt_path(backup_filepath, snap_dir, filename);
          
          // Issue xv6 system call to delete the cached copy file
          unlink(backup_filepath);
        }
        line_start = &buf[i + 1];
      }
    }
  }
  
  close(fd_meta);
  
  // Wipe out the internal tracking register file 
  unlink(meta_path);
  
  // Attempt to remove the snapshot directory node itself
  if(unlink(snap_dir) < 0) {
    printf("Note: Run 'rm %s' inside shell if your current Xv6 build requires manual directory unlinking.\n", snap_dir);
  } else {
    printf("Snapshot directory '%s' dropped completely.\n", snap_dir);
  }
  
  printf("Snapshot '%s' successfully deleted.\n", snap_name);
}