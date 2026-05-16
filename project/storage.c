// === TASK 3: STORAGE MANAGER & METADATA WRITER ===
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "utils.h"
#include "copy.h"
#include "storage.h"

static void snapshot_and_log_recursive(char *current_path, char *snap_dest_dir, int meta_fd)
{
  char buf_src[512];
  char buf_dest[512];
  char log_line[256];
  char name[DIRSIZ + 1];

  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(current_path, O_RDONLY)) < 0)
    return;

  if (fstat(fd, &st) < 0)
  {
    close(fd);
    return;
  }

  if (st.type == T_DIR)
  {

    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {

      if (de.inum == 0)
        continue;

      // SAFELY copy directory name
      memmove(name, de.name, DIRSIZ);
      name[DIRSIZ] = 0;

      // skip special folders
      if (strcmp(name, ".") == 0 ||
          strcmp(name, "..") == 0 ||
          strcmp(name, "snapshots") == 0)
        continue;

      // build source path
      fmt_path(buf_src, current_path, name);

      int child_fd = open(buf_src, O_RDONLY);

      if (child_fd < 0)
        continue;

      if (fstat(child_fd, &st) >= 0)
      {

        if (st.type == T_FILE)
        {

          // build destination path
          fmt_path(buf_dest, snap_dest_dir, name);

          // copy file
          copy_file(buf_src, buf_dest);

          // write metadata using FULL relative path
          int path_len = strlen(buf_src);

          memmove(log_line, buf_src, path_len);

          log_line[path_len] = ',';

          char size_str[16];
          int size = (int)st.size;
          int i = 0;

          if (size == 0)
            size_str[i++] = '0';

          while (size > 0 && i < 15)
          {
            size_str[i++] = (size % 10) + '0';
            size = size / 10;
          }

          size_str[i] = '\0';

          // reverse digits
          for (int j = 0; j < i / 2; j++)
          {
            char t = size_str[j];
            size_str[j] = size_str[i - 1 - j];
            size_str[i - 1 - j] = t;
          }

          strcpy(log_line + path_len + 1, size_str);

          int complete_len = strlen(log_line);

          log_line[complete_len] = '\n';
          log_line[complete_len + 1] = '\0';

          write(meta_fd, log_line, strlen(log_line));
        }
        else if (st.type == T_DIR)
        {

          close(child_fd);

          snapshot_and_log_recursive(buf_src, snap_dest_dir, meta_fd);

          continue;
        }
      }

      close(child_fd);
    }
  }

  close(fd);
}

void run_create(char *snapshot_name, char *target_path)
{
  char snap_root[] = "snapshots";
  char snap_folder[256];
  char meta_filepath[512];
  int meta_fd;

  mkdir(snap_root);
  fmt_path(snap_folder, snap_root, snapshot_name);

  if (mkdir(snap_folder) < 0)
  {
    printf("snapshot create: snapshot '%s' already exists\n", snapshot_name);
    return;
  }

  fmt_path(meta_filepath, snap_folder, "snapshot.meta");
  if ((meta_fd = open(meta_filepath, O_WRONLY | O_CREATE | O_TRUNC)) < 0)
  {
    printf("snapshot create: failed to generate metadata ledger\n");
    return;
  }

  printf("Creating snapshot and metadata ledger inside: %s...\n", snap_folder);
  snapshot_and_log_recursive(target_path, snap_folder, meta_fd);
  close(meta_fd);
  printf("Snapshot '%s' and its 'snapshot.meta' ledger successfully recorded.\n", snapshot_name);
}
