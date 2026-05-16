// === TASK 5: RESTORE SYSTEM ===
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "utils.h"
#include "copy.h"
#include "restore.h"

void run_restore(char *snap_name)
{
  char meta_path[512];
  char snap_dir[256];
  int fd_meta;
  char buf[2048];
  int n;

  fmt_path(snap_dir, "snapshots", snap_name);
  fmt_path(meta_path, snap_dir, "snapshot.meta");

  if ((fd_meta = open(meta_path, O_RDONLY)) < 0)
  {
    printf("snapshot restore: snapshot ledger '%s' not found\n", meta_path);
    return;
  }

  printf("Restoring file system state from snapshot '%s'...\n", snap_name);

  if ((n = read(fd_meta, buf, sizeof(buf) - 1)) > 0)
  {
    buf[n] = '\0';

    char *line_start = buf;
    for (int i = 0; i <= n; i++)
    {
      if (buf[i] == '\n' || buf[i] == '\0')
      {
        if (buf[i] == '\n')
          buf[i] = '\0';

        char *comma = 0;
        for (char *c = line_start; *c != '\0'; c++)
        {
          if (*c == ',')
          {
            comma = c;
            break;
          }
        }

        if (comma != 0)
        {
          *comma = '\0';
          char *filename = line_start;

          char backup_filepath[512];
          char live_filepath[512];

          char *base = filename;

          // extract basename
          for (char *p = filename; *p != '\0'; p++)
          {
            if (*p == '/')
              base = p + 1;
          }

          // backup file inside snapshot
          fmt_path(backup_filepath, snap_dir, base);

          // original live file path
          strcpy(live_filepath, filename);

          copy_file(backup_filepath, live_filepath);

          printf("Restored: %s\n", filename);
        }
        line_start = &buf[i + 1];
      }
    }
  }
  close(fd_meta);
  printf("System restoration complete.\n");
}
