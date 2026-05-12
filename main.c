#include <stdio.h>
#include <time.h>
#include "scanner.h"
#include "snapshot.h"
#include "compare.h"

int main() {
    const char *source_dir      = "/home/abdullrhman/linux-file-system-/file_project";
    const char *snapshot_folder = "/home/abdullrhman/linux-file-system-/snapshots/snapshot_1";
    const char *meta_path       = "/home/abdullrhman/linux-file-system-/snapshots/snapshot_1/snapshot.meta";

    FileInfo files[1000];
    int count;

    scan_directory(source_dir, files, &count);

    printf("Scanned files:\n");
    int i;
    for (i = 0; i < count; i++) {
        char timebuf[100];
        struct tm *tm_info = localtime(&files[i].modified_time);
        strftime(timebuf, 100, "%Y-%m-%d %H:%M", tm_info);
        printf("  %s | %ld bytes | %s\n", files[i].path, files[i].size, timebuf);
    }

    printf("\nCreating snapshot...\n");
    create_snapshot(source_dir, snapshot_folder, files, count);

    printf("\nComparing snapshot against live directory...\n");

    FileInfo baseline[1000];
    int base_count;

    if (load_snapshot_meta(meta_path, baseline, &base_count) != 0) {
        printf("Could not load snapshot meta. Skipping comparison.\n");
        return 1;
    }

    FileInfo current[1000];
    int curr_count;
    scan_directory(source_dir, current, &curr_count);

    CompareResult result;
    compare_snapshots(baseline, base_count, current, curr_count, &result);
    print_compare_result(&result);

    return 0;
}
