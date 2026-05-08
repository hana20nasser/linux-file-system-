#include <stdio.h>
#include <string.h>
#include "compare.h"

int load_snapshot_meta(const char *meta_path, FileInfo files[], int *count) {
    *count = 0;

    FILE *f = fopen(meta_path, "r");
    if (!f) {
        perror("load_snapshot_meta: cannot open meta file");
        return -1;
    }

    char line[800];

    while (fgets(line, sizeof(line), f)) {

        if (strncmp(line, "created_at", 10) == 0) {
            continue;
        }

        if (line[0] == '\n' || line[0] == '\0') {
            continue;
        }

        char path[MAX_PATH_LEN];
        long size;
        long mtime;

        if (sscanf(line, "%255s %ld %ld", path, &size, &mtime) == 3) {

            if (*count >= MAX_FILES) {
                printf("load_snapshot_meta: too many files, stopping at %d\n", MAX_FILES);
                break;
            }

            strncpy(files[*count].path, path, MAX_PATH_LEN - 1);
            files[*count].path[MAX_PATH_LEN - 1] = '\0';
            files[*count].size          = size;
            files[*count].modified_time = mtime;
            (*count)++;

        } else {
            printf("load_snapshot_meta: skipping unrecognised line: %s", line);
        }
    }

    fclose(f);
    return 0;
}


static int find_in_list(const char *path, FileInfo *list, int list_count) {
    int i;
    for (i = 0; i < list_count; i++) {
        if (strcmp(path, list[i].path) == 0) {
            return i;
        }
    }
    return -1;
}


void compare_snapshots(FileInfo *baseline, int base_count,
                       FileInfo *current,  int curr_count,
                       CompareResult *result) {

    int i, j;

    result->added_count    = 0;
    result->modified_count = 0;
    result->deleted_count  = 0;

    for (i = 0; i < base_count; i++) {

        j = find_in_list(baseline[i].path, current, curr_count);

        if (j == -1) {
            if (result->deleted_count < MAX_FILES) {
                strncpy(result->deleted[result->deleted_count],
                        baseline[i].path, MAX_PATH_LEN - 1);
                result->deleted[result->deleted_count][MAX_PATH_LEN - 1] = '\0';
                result->deleted_count++;
            }

        } else {
            int size_changed  = (baseline[i].size          != current[j].size);
            int mtime_changed = (baseline[i].modified_time != current[j].modified_time);

            if (size_changed || mtime_changed) {
                if (result->modified_count < MAX_FILES) {
                    strncpy(result->modified[result->modified_count],
                            baseline[i].path, MAX_PATH_LEN - 1);
                    result->modified[result->modified_count][MAX_PATH_LEN - 1] = '\0';
                    result->modified_count++;
                }
            }
        }
    }

    for (i = 0; i < curr_count; i++) {

        j = find_in_list(current[i].path, baseline, base_count);

        if (j == -1) {
            if (result->added_count < MAX_FILES) {
                strncpy(result->added[result->added_count],
                        current[i].path, MAX_PATH_LEN - 1);
                result->added[result->added_count][MAX_PATH_LEN - 1] = '\0';
                result->added_count++;
            }
        }
    }
}


void print_compare_result(const CompareResult *result) {
    int i;

    printf("\n========== Comparison Report ==========\n");

    printf("\n[Added]  (%d file(s))\n", result->added_count);
    if (result->added_count == 0) {
        printf("  (none)\n");
    } else {
        for (i = 0; i < result->added_count; i++) {
            printf("  + %s\n", result->added[i]);
        }
    }

    printf("\n[Modified]  (%d file(s))\n", result->modified_count);
    if (result->modified_count == 0) {
        printf("  (none)\n");
    } else {
        for (i = 0; i < result->modified_count; i++) {
            printf("  ~ %s\n", result->modified[i]);
        }
    }

    printf("\n[Deleted]  (%d file(s))\n", result->deleted_count);
    if (result->deleted_count == 0) {
        printf("  (none)\n");
    } else {
        for (i = 0; i < result->deleted_count; i++) {
            printf("  - %s\n", result->deleted[i]);
        }
    }

    printf("\n=======================================\n");
}
