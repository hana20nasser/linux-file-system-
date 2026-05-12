#include <stdio.h>
#include <string.h>
#include <stdlib.h>      
#include <unistd.h>    
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include "storage.h"

static pthread_mutex_t snapshot_lock = PTHREAD_MUTEX_INITIALIZER;

/* ── generate_snapshot_name ───────────────────────────────────────── */
void generate_snapshot_name(char *out_name, int len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(out_name, len, "%Y-%m-%d_%H-%M", t);
}

/* ── save_snapshot ────────────────────────────────────────────────── */
int save_snapshot(const char *snapshots_root,
                  const char *snap_name,
                  FileInfo files[], int count) {

    pthread_mutex_lock(&snapshot_lock);

    char snap_dir[512];
    snprintf(snap_dir, sizeof(snap_dir), "%s/%s", snapshots_root, snap_name);
    mkdir(snap_dir, 0755);

    char meta_path[512];
    snprintf(meta_path, sizeof(meta_path), "%s/snapshot.meta", snap_dir);

    FILE *f = fopen(meta_path, "w");
    if (!f) {
        perror("save_snapshot");
        pthread_mutex_unlock(&snapshot_lock);
        return -1;
    }

    fprintf(f, "created_at:%ld\n", (long)time(NULL));

    int i;
    for (i = 0; i < count; i++) {
        fprintf(f, "%s %ld %ld\n",
                files[i].path,
                files[i].size,
                files[i].modified_time);
    }

    fclose(f);
    printf("Snapshot saved: %s (%d files)\n", snap_dir, count);

    pthread_mutex_unlock(&snapshot_lock);
    return 0;
}

/* ── list_snapshots ───────────────────────────────────────────────── */
void list_snapshots(const char *snapshots_root) {

    pthread_mutex_lock(&snapshot_lock);

    DIR *dir = opendir(snapshots_root);
    if (!dir) {
        printf("No snapshots found.\n");
        pthread_mutex_unlock(&snapshot_lock);
        return;
    }

    struct dirent *entry;
    int found = 0;
    printf("\n===== Snapshots =====\n");

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char meta_path[512];
        snprintf(meta_path, sizeof(meta_path), "%s/%s/snapshot.meta",
                 snapshots_root, entry->d_name);

        FILE *meta = fopen(meta_path, "r");
        if (!meta) continue;

        char line[256];
        char created_at[64] = "unknown";

        while (fgets(line, sizeof(line), meta)) {
            if (strncmp(line, "created_at:", 11) == 0) {
                long ts = atol(line + 11);
                struct tm *t = localtime(&ts);
                strftime(created_at, sizeof(created_at), "%Y-%m-%d %H:%M", t);
                break;
            }
        }
        fclose(meta);
        printf("  [%s] created at %s\n", entry->d_name, created_at);
        found++;
    }

    if (!found) printf("  No snapshots found.\n");
    closedir(dir);
    printf("=====================\n\n");

    pthread_mutex_unlock(&snapshot_lock);
}

/* ── delete_snapshot_dir (helper: recursively delete a directory) ─── */
static void delete_dir_recursive(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char full_path[512];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) continue;

        if (S_ISDIR(st.st_mode))
            delete_dir_recursive(full_path);
        else
            remove(full_path);
    }

    closedir(dir);
    rmdir(path);
}

/* ── delete_snapshot ──────────────────────────────────────────────── */
int delete_snapshot(const char *snapshots_root, const char *snap_name) {

    pthread_mutex_lock(&snapshot_lock);

    char snap_path[512];
    snprintf(snap_path, sizeof(snap_path), "%s/%s", snapshots_root, snap_name);

    struct stat st;
    if (stat(snap_path, &st) == -1) {
        printf("Snapshot '%s' not found.\n", snap_name);
        pthread_mutex_unlock(&snapshot_lock);
        return -1;
    }

    delete_dir_recursive(snap_path);
    printf("Snapshot '%s' deleted.\n", snap_name);

    pthread_mutex_unlock(&snapshot_lock);
    return 0;
}

/* ── enforce_limit ────────────────────────────────────────────────── */
void enforce_limit(const char *snapshots_root, int max_count) {
    DIR *dir = opendir(snapshots_root);
    if (!dir) return;

    char  names[MAX_SNAPSHOTS][256];
    long  times[MAX_SNAPSHOTS];
    int   total = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char meta_path[512];
        snprintf(meta_path, sizeof(meta_path), "%s/%s/snapshot.meta",
                 snapshots_root, entry->d_name);

        FILE *meta = fopen(meta_path, "r");
        if (!meta) continue;

        char line[256];
        long ts = 0;
        while (fgets(line, sizeof(line), meta)) {
            if (strncmp(line, "created_at:", 11) == 0) {
                ts = atol(line + 11);
                break;
            }
        }
        fclose(meta);

        strncpy(names[total], entry->d_name, 255);
        times[total] = ts;
        total++;
    }
    closedir(dir);

    while (total > max_count) {
        int oldest = 0;
        int i;
        for (i = 1; i < total; i++) {
            if (times[i] < times[oldest]) oldest = i;
        }
        delete_snapshot(snapshots_root, names[oldest]);
        for (i = oldest; i < total - 1; i++) {
            strncpy(names[i], names[i+1], 255);
            times[i] = times[i+1];
        }
        total--;
    }
}