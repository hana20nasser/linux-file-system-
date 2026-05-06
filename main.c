#include <stdio.h>
#include <time.h>
#include "scanner.h"
#include "snapshot.h"

int main() {
    FileInfo files[1000];
    int count;

    /*
        source_dir:
        This is the folder that Task 1 will scan.
    */
    const char *source_dir = "/home/seif-ismail/os_project/file_project";

    /*
        snapshot_folder:
        This is where Task 2 will create the snapshot.
    */
    const char *snapshot_folder = "/home/seif-ismail/os_project/snapshots/snapshot_1";

    /*
        Task 1:
        Scan the source directory and store file info in files[].
    */
    scan_directory(source_dir, files, &count);

    /*
        Print scanned files.
    */
    printf("Scanned files:\n");

    for (int i = 0; i < count; i++) {
        char timebuf[100];

        struct tm *tm_info = localtime(&files[i].modified_time);
        strftime(timebuf, 100, "%Y-%m-%d %H:%M", tm_info);

        printf("%s | %ld bytes | %s\n",
               files[i].path,
               files[i].size,
               timebuf);
    }

    /*
        Task 2:(SEIF)
        Create snapshot using the scanned files.
    */
    printf("\nCreating snapshot...\n");

    create_snapshot(source_dir, snapshot_folder, files, count);

    return 0;
}
