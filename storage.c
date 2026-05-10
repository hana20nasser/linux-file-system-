#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ftw.h>
#include "storage.h"

void generate_snapshot_name(char *out_name, int len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(out_name, len, "%Y-%m-%d_%H-%M", t);
}

static int mkdir_p(const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755);
}

int save_snapshot(const char *snapshots_root,
                  const char *snap_name,
                  FileInfo files[], int count) {

    char snap_dir[512];
    snprintf(snap_dir, sizeof(snap_dir), "%s/%s", snapshots_root, snap_name);
    mkdir_p(snap_dir);

    char meta_path[512];
    snprintf(meta_path, sizeof(meta_path), "%s/snapshot.meta", snap_dir);

    FILE *f = fopen(meta_path, "w");
    if (!f) {
        perror("save_snapshot: cannot open snapshot.meta");
        return -1;
    }

    fprintf(f, "created_at:%ld\n", (long)time(NULL));
    for (int i = 0; i < count; i++) {
        fprintf(f, "%s %ld %ld\n",
                files[i].path,
                files[i].size,
                files[i].modified_time);
    }

    fclose(f);
    printf("Snapshot saved: %s (%d files)\n", snap_dir, count);
    return 0;
}