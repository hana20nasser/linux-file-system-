#ifndef COMPARE_H
#define COMPARE_H

#include "scanner.h"

#define MAX_FILES     1000
#define MAX_PATH_LEN  256

/* Holds the results of a comparison between a snapshot and the live directory */
typedef struct {
    char added[MAX_FILES][MAX_PATH_LEN];
    int  added_count;

    char modified[MAX_FILES][MAX_PATH_LEN];
    int  modified_count;

    char deleted[MAX_FILES][MAX_PATH_LEN];
    int  deleted_count;
} CompareResult;

/* Reads snapshot.meta into a FileInfo array.
   Returns 0 on success, -1 on failure. */
int load_snapshot_meta(const char *meta_path, FileInfo files[], int *count);

/* Compares baseline (from snapshot) vs current (live scan).
   Fills result with Added / Modified / Deleted lists. */
void compare_snapshots(FileInfo *baseline, int base_count,
                       FileInfo *current,  int curr_count,
                       CompareResult *result);

/* Prints the CompareResult to stdout in a readable format. */
void print_compare_result(const CompareResult *result);

#endif
