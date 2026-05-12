#ifndef STORAGE_H
#define STORAGE_H

#include "scanner.h"
#include <dirent.h>
#include <sys/stat.h>

#define SNAPSHOTS_ROOT "/home/abdullrhman/linux-file-system-/snapshots"
#define MAX_SNAPSHOTS  10

/* Generate a timestamp-based snapshot name */
void generate_snapshot_name (char *out_name, int len);

/* Create snapshot folder and write snapshot.meta */
int save_snapshot(const char *snapshots_root,
                  const char *snap_name,
                  FileInfo files[], int count);

/* List all snapshots with their creation time */
void list_snapshots(const char *snapshots_root);

/* Delete a named snapshot folder entirely */
int delete_snapshot(const char *snapshots_root, const char *snap_name);

/* Delete oldest snapshots if total exceeds max_count */
void enforce_limit(const char *snapshots_root, int max_count);

#endif