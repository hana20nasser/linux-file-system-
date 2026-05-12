# File System Snapshot & Restore — Project README

> A lightweight, Timeshift-like utility written in C that snapshots a directory tree and restores it to a previous state.

---

## Project Status

| Task | Description | Status |
| ---- | ----------- | ------ |
| Task 1 | File System Scanner | ✅ COMPLETE |
| Task 2 | Snapshot Creator | ✅ COMPLETE |
| Task 3 | Snapshot Storage Manager | 🟡 PARTIALLY IMPLEMENTED |
| Task 4 | Change Detection (Comparison Engine) | ✅ COMPLETE |
| Task 5 | Restore System | 🔲 Not started |

---

## Repository Layout

```
.
├── main.c          # Entry point — currently drives Tasks 1, 2, and 4 in sequence
├── scanner.h       # Public API and FileInfo struct definition
├── scanner.c       # Recursive directory walker (Task 1)
├── snapshot.h      # Public API for snapshot creation
├── snapshot.c      # Snapshot creator implementation (Task 2)
├── storage.h       # Public API for snapshot storage manager (Task 3)
├── storage.c       # Snapshot storage manager implementation (Task 3, partial)
├── compare.h       # Public API for change detection
├── compare.c       # Comparison engine implementation (Task 4)
├── snapshots/      # Directory where snapshots are stored
│   └── snapshot_1/
│       ├── snapshot.meta   ← metadata index for this snapshot
│       └── <copied files>  ← mirrored directory tree
└── README.md       # This file
```

---

## What Has Been Built

### Task 1 — File System Scanner ✅

**Files:** `scanner.h`, `scanner.c`

**Purpose:** Recursively walk a directory tree, collect metadata for every regular file, and return the results in a flat array of `FileInfo` structs.

**Public API (`scanner.h`):**

```c
typedef struct {
    char path[256];        // Absolute path to the file
    long size;             // File size in bytes
    long modified_time;    // Last-modified timestamp (Unix epoch via st_mtime)
} FileInfo;

int scan_directory(const char *path, FileInfo files[], int *count);
```

**How it works:**

`scan_directory` initialises `*count` to zero and calls the internal `scan_recursive` helper. That helper uses POSIX `opendir` / `readdir` to iterate each directory entry, skips `.` and `..`, builds an absolute path, and stats the entry. Regular files get their path, size, and `st_mtime` written into the `files` array. Directories trigger a recursive call on themselves.

**Output format:**

```
/home/user/file_project/foo.c  | 2048 bytes | 2026-04-30 14:22
```

The timestamp is formatted with `strftime("%Y-%m-%d %H:%M")`.

---

### Task 2 — Snapshot Creator ✅

**Files:** `snapshot.h`, `snapshot.c`

**Purpose:** Take the `FileInfo` array produced by Task 1 and physically copy every file into a versioned snapshot directory, while writing a machine-readable metadata index (`snapshot.meta`) that Tasks 3, 4, and 5 depend on.

**Public API (`snapshot.h`):**

```c
int create_snapshot(const char *source_dir, const char *snapshot_folder,
                    FileInfo files[], int count);
```

**How it works:**

`create_snapshot` does the following steps in order:

1. **Creates the snapshot directory** using `create_directories()`, which iterates the path character by character and calls `mkdir(path, 0777)` on each prefix — this gives `mkdir -p` semantics without relying on the shell.

2. **Opens `snapshot.meta`** for writing (truncated fresh each run) inside the snapshot folder.

3. **Iterates every `FileInfo`** in the array:
   - Strips the source directory prefix from the absolute path using `get_relative_path()` to get a path like `subdir/file.c`.
   - Builds the destination path as `snapshot_folder/subdir/file.c`.
   - Calls `copy_file()` which opens the source with `O_RDONLY`, creates any missing parent directories, opens the destination with `O_WRONLY | O_CREAT | O_TRUNC`, and copies content in 1 KB chunks using `read` / `write`.
   - On success, writes one line to `snapshot.meta` in the format:
     ```
     /absolute/path/to/original/file <size> <mtime>
     ```

**`snapshot.meta` format (current):**

```
/home/user/file_project/main.c 3200 1745990000
/home/user/file_project/foo.c 2048 1745991234
```

Each line is one file: absolute path, size in bytes, and last-modified time — all space-separated. `load_snapshot_meta()` also tolerates a `created_at <epoch>` header line if it is added later.

**Example output:**

```
Copied: /home/user/file_project/main.c
Copied: /home/user/file_project/foo.c

Snapshot created successfully in: /home/user/snapshots/snapshot_1
```

---

### Task 3 — Snapshot Storage Manager 🟡

**Files:** `storage.h`, `storage.c`

**Purpose:** Organize and persist snapshots under a central snapshots root, with unique timestamp-based names and snapshot metadata.

**Public API (`storage.h`):**

```c
void generate_snapshot_name (char *out_name, int len);

int save_snapshot(const char *snapshots_root,
                  const char *snap_name,
                  FileInfo files[], int count);

void list_snapshots(const char *snapshots_root);
int delete_snapshot(const char *snapshots_root, const char *snap_name);
void enforce_limit(const char *snapshots_root, int max_count);
```

**What is implemented now (`storage.c`):**

- `generate_snapshot_name()` creates names like `YYYY-MM-DD_HH-MM` using `strftime`.
- `save_snapshot()` creates `<snapshots_root>/<snap_name>/snapshot.meta`, writes `created_at`, then stores `<path> <size> <mtime>` for each `FileInfo`.
- Internal helper `mkdir_p()` recursively creates missing directories.

**What is still missing for full Task 3 completion:**

- `list_snapshots()` body
- `delete_snapshot()` body
- `enforce_limit()` body

---

### Task 4 — Change Detection (Comparison Engine) ✅

**Files:** `compare.h`, `compare.c`

**Purpose:** Compare the live state of a directory against a saved snapshot and report which files were Added, Modified, or Deleted.

**Public API (`compare.h`):**

```c
typedef struct {
    char added[1000][256];
    int  added_count;

    char modified[1000][256];
    int  modified_count;

    char deleted[1000][256];
    int  deleted_count;
} CompareResult;

int  load_snapshot_meta(const char *meta_path, FileInfo files[], int *count);
void compare_snapshots(FileInfo *baseline, int base_count,
                       FileInfo *current,  int curr_count,
                       CompareResult *result);
void print_compare_result(const CompareResult *result);
```

**How it works:**

The comparison runs in three stages:

**Stage 1 — Load the baseline from `snapshot.meta`**

`load_snapshot_meta()` opens the meta file and reads it line by line using `fgets`. It skips a `created_at` header line if present and any blank lines. For every remaining line it uses `sscanf` to parse the path, size, and mtime into a `FileInfo` struct, building up the baseline array. This is the inverse of what `snapshot.c` writes.

**Stage 2 — Scan the live directory**

`scan_directory()` from Task 1 is called again on the source directory to get the current state as a fresh `FileInfo` array.

**Stage 3 — Compare the two arrays**

`compare_snapshots()` uses an internal helper `find_in_list()` which does a linear search by path string using `strcmp`. The comparison runs in two passes:

- **Pass 1 (walk the baseline):** For each file in the snapshot, search for it in the live array.
  - Not found → **Deleted**
  - Found but `size` or `modified_time` differs → **Modified**
  - Found and identical → no change

- **Pass 2 (walk the live directory):** For each file in the live array, search for it in the baseline.
  - Not found → **Added**

All matching is done on absolute path. A file is only considered the same file if the path string is identical.

**Output format:**

```
========== Comparison Report ==========

[Added]  (1 file(s))
  + /home/user/file_project/newfile.txt

[Modified]  (1 file(s))
  ~ /home/user/file_project/main.c

[Deleted]  (1 file(s))
  - /home/user/file_project/old.c

=======================================
```

**Known limitation:** Matching is based on absolute path, so if the source directory is moved or renamed between snapshot and comparison, all files will show as Deleted + Added rather than Unchanged.

---

## How the Current Runtime Flow Connects in `main.c`

```
main.c
  │
  ├─ Task 1: scan_directory(source_dir)
  │         → fills files[] with live FileInfo
  │
  ├─ Task 2: create_snapshot(source_dir, snapshot_folder, files, count)
  │         → copies files into snapshots/snapshot_1/
  │         → writes snapshots/snapshot_1/snapshot.meta
  │
  └─ Task 4: load_snapshot_meta(meta_path, baseline, &base_count)
             scan_directory(source_dir, current, &curr_count)
             compare_snapshots(baseline, base_count, current, curr_count, &result)
             print_compare_result(&result)
```

Because all three run in the same execution right now, the comparison will always show no changes on the first run (you are comparing the snapshot to the directory it was just taken from). To see real diffs:

1. Run the program once — the snapshot is created.
2. Add, edit, or delete a file inside `source_dir`.
3. Run the program again — Task 4 will detect and report the changes.

---

## Known Issues & Technical Debt

| # | Task Holder | Location | Issue | Severity |
| --- | --- | --- | --- | --- |
| 1 | Task 1 | `scanner.c:18` | `stat()` is called **twice** on every path — the first result is silently discarded. Remove the first call. | Medium |
| 2 | Task 1 | `scanner.h` | `path[256]` is too short for deeply nested trees. Increase to **512** to match the internal buffer in `scanner.c`. A mismatch causes silent buffer overflows. | High |
| 3 | Task 1 | `scanner.c` | `sprintf` for path construction has no bounds check. Replace with `snprintf(path, sizeof(path), ...)`. | High |
| 4 | Task 4 | `main.c` | The `files[1000]` hard-limit silently truncates results beyond 1000 files. The limit should be passed into `scan_directory` and checked inside `scan_recursive`. | Medium |
| 5 | Task 1 | `scanner.c` | Symbolic links are neither followed nor reported. Decide on a policy (skip or dereference) and document it. | Low |
| 6 | Task 2 | `snapshot.c` | `sprintf` in `create_snapshot` writes into a 700-byte `line` buffer but paths can be up to 256 bytes — safe today but fragile. Replace with `snprintf`. | Medium |
| 7 | Task 2 | `snapshot.c` | Return values from `write()` in `copy_file()` and while writing to `snapshot.meta` are not checked. Disk-full or partial writes will fail silently. | Medium |
| 8 | Task 2 | `snapshot.c` | `snapshot.meta` does not yet include the `created_at` timestamp line. Add `time(NULL)` as the first line before the file loop — required for Task 3's `list_snapshots`. | Medium |
| 9 | Task 4 | `compare.c` | `load_snapshot_meta()` uses space-delimited parsing (`%s`), so paths containing spaces cannot be read reliably. | Low |
| 10 | Task 4 | `main.c` | Paths are hard-coded to a specific machine. These need to be adjusted before running on a new environment. | Low |
| 11 | Task 4 | `compare.c` | `find_in_list` is O(n²) — fine for small directories, but will be slow for thousands of files. Could be improved with sorting + binary search if needed. | Low |

---

## Architecture Overview

```
┌─────────────────┐     ┌──────────────────┐     ┌───────────────────┐
│  Task 1: Scan   │────▶│ Task 2: Snapshot  │────▶│ Task 4: Compare   │
│  ✅ COMPLETE    │     │  ✅ COMPLETE      │     │  ✅ COMPLETE      │
└─────────────────┘     └──────────────────┘     └───────────────────┘
                                 │
                                 ▼
                        ┌────────────────────────────┐
                        │ Task 3: Snapshot Storage   │
                        │ Manager                    │
                        │ 🟡 PARTIALLY IMPLEMENTED   │
                        └────────────────────────────┘
                                 │
                                 ▼
                        ┌──────────────────┐
                        │ Task 5: Restore  │
                        │  🔲 Not started  │
                        └──────────────────┘
```

---

## Tasks 3 and 5 Specifications

### Task 3 — Snapshot Storage Manager (CLI)

**Goal:** Provide a command-line interface to list and delete snapshots.

**Supported commands:**

```
list_snapshots                  → prints all snapshots with creation time
delete_snapshot <name>          → removes the snapshot directory and meta file
create_snapshot <dir> <name>    → wraps Task 2
```

**Expected output:**

```
snapshot_1 - created at 2026-04-30
snapshot_2 - created at 2026-04-29
Snapshot snapshot_1 deleted successfully.
```

**Implementation notes:**

- Snapshot creation time is stored on the `created_at` line of `snapshot.meta` (see Issue #8 above — must be added to `snapshot.c` first).
- `list_snapshots` reads the top-level `/snapshots/` directory, opens each `snapshot.meta`, and prints the name and timestamp.
- `delete_snapshot` uses `nftw` with `FTW_DEPTH | FTW_PHYS` to recursively `unlink` files then `rmdir` directories.
- All commands should be reachable from `main.c` via `argv` parsing.
- Storage functionality currently lives in `storage.h` / `storage.c`.

---

### Task 5 — Restore System

**Goal:** Overwrite the target directory with the contents of a snapshot, leaving it byte-for-byte identical to the snapshotted state.

**Inputs:** A snapshot name and a target directory.

**Expected behaviour:**

- Files present in the snapshot but missing live → **copied in**.
- Files present live but absent from the snapshot → **deleted**.
- Files present in both but modified → **overwritten** with the snapshot version.
- Permissions and `mtime` should be restored where possible (`utimes`, `chmod`).

**Safety requirements:**

- Before destructive operations, print a summary of what will change and prompt `y/N` for confirmation.
- Never operate outside the target directory (validate all paths with `realpath` and check the prefix).
- Consider writing a temporary staging area and doing an atomic swap to avoid leaving a half-restored state on error.
- Suggested new files: `restore.h`, `restore.c`.

---

## Build Instructions

```bash
# Tasks 1, 2, and 4 (current runtime flow)
gcc -o project main.c scanner.c snapshot.c compare.c -Wall -Wextra

# After completing Tasks 3 and 5 integration
gcc -o project main.c scanner.c snapshot.c compare.c storage.c restore.c -Wall -Wextra
```

No external dependencies beyond a standard POSIX C library are required for the core tasks. Optional SHA-256 hashing for Task 4 advanced mode links against `-lssl` (`openssl/sha.h`).

---

## Suggested Development Order for Remaining Tasks

1. **Fix known issues** listed above (especially #2, #3, #8) before writing new code — they affect correctness of Tasks 3 and 5.
2. Complete **Task 3** (storage manager): implement `list_snapshots`, `delete_snapshot`, and `enforce_limit` in `storage.c`, then wire CLI commands in `main.c`.
3. Implement **Task 5** (restore) — last because it is the most destructive operation and should be tested against known-good snapshots produced by Tasks 2–4.

---

## Contributing

- Follow the existing code style: POSIX C99, no dynamic allocation unless necessary.
- Each new module gets a `.h` / `.c` pair and a corresponding section in this README.
- Test against both shallow and deeply nested directory trees.
- All file operations must handle error returns — `perror` + `continue`/`return` is the established pattern in this codebase.
