# File System Snapshot & Restore — Project README

> A lightweight, Timeshift-like utility written in C that snapshots a directory tree and restores it to a previous state.

---

## Project Status

**Task 1 (File System Scanner) — ✅ COMPLETE**

All remaining tasks (2–5) are **not yet started**. This document describes what has been built, the exact state of the codebase, known issues, and the full specification for what contributors need to implement next.

---

## Repository Layout

```
.
├── main.c          # Entry point — drives the scanner and prints results
├── scanner.h       # Public API and FileInfo struct definition
├── scanner.c       # Recursive directory walker (Task 1 implementation)
└── README.md       # This file
```

---

## What Has Been Built — Task 1: File System Scanner

### Purpose
Recursively walk a directory tree, collect metadata for every regular file, and return the results in a flat array of `FileInfo` structs.

### Public API (`scanner.h`)

```c
typedef struct {
    char path[256];        // Absolute path to the file
    long size;             // File size in bytes
    long modified_time;    // Last-modified timestamp (Unix epoch via st_mtime)
} FileInfo;

int scan_directory(const char *path, FileInfo files[], int *count);
```

`scan_directory` initialises `*count` to zero, delegates to the internal recursive helper, and always returns 0. The caller supplies a pre-allocated array (`FileInfo files[1000]` in `main.c`).

### Internal Implementation (`scanner.c`)

`scan_recursive` uses POSIX `opendir` / `readdir` to iterate each directory entry, skips `.` and `..`, builds an absolute path with `sprintf`, stats the entry, and then:

- **Regular file** → writes path, size, and `st_mtime` into `files[*count]` and increments the counter.
- **Directory** → calls itself recursively.

### Current Output Format (`main.c`)

```
/home/vboxuser/file_project/foo.c  | 2048 bytes | 2026-04-30 14:22
```

The timestamp is formatted with `strftime("%Y-%m-%d %H:%M")`.

---

## Known Issues & Technical Debt

These must be addressed before or alongside new task work:

| # | Location | Issue | Severity |
|---|----------|-------|----------|
| 1 | `scanner.c:18` | `stat()` is called **twice** on every path — once before the error check and once again (the first call result is silently discarded). Remove the first call. | Medium |
| 2 | `scanner.h` | `path[256]` is too short for deeply nested trees. Increase to **512** to match the `char path[512]` buffer already used in `scanner.c`. A mismatch here causes silent buffer overflows. | High |
| 3 | `scanner.c` | `sprintf` for path construction has no bounds check. Replace with `snprintf(path, sizeof(path), ...)`. | High |
| 4 | `main.c` | The `files[1000]` hard-limit means the scanner silently truncates results beyond 1000 files. The limit should be passed into `scan_directory` and checked inside `scan_recursive`. | Medium |
| 5 | `scanner.c` | Symbolic links are neither followed nor reported. Decide on a policy (skip or dereference) and document it. | Low |

---

## Architecture Overview

The system is designed around five sequential tasks. Only Task 1 is done. Tasks 2–5 are the contributor's responsibility.

```
┌─────────────────┐     ┌──────────────────┐     ┌───────────────────┐
│  Task 1: Scan   │────▶│ Task 2: Snapshot  │────▶│ Task 3: Compare   │
│  (COMPLETE)     │     │  Creator          │     │  Engine           │
└─────────────────┘     └──────────────────┘     └───────────────────┘
                                 │                         │
                                 ▼                         ▼
                        ┌──────────────────┐     ┌───────────────────┐
                        │ Task 5: Restore  │◀────│ Task 4: Snapshot  │
                        │  System          │     │  Manager (CLI)    │
                        └──────────────────┘     └───────────────────┘
```

---

## Tasks 2–5 Specifications

### Task 2 — Snapshot Creator

**Goal:** Copy the scanned files into a versioned snapshot directory and persist the metadata.

**Inputs:** A source directory path and a snapshot name (e.g., `snap1`).

**Expected output:**
```
/snapshots/
  snap1/
    files/          ← physical copies of every scanned file, mirroring original paths
    snapshot.meta   ← JSON or plain-text index (path, size, mtime for each file)
```

**Implementation notes:**
- Use the `FileInfo` array produced by `scan_directory` to know what to copy.
- Reproduce the original subdirectory structure under `files/` so restore is straightforward.
- `snapshot.meta` must be machine-readable — it is consumed by Tasks 3 and 5.
- Use `mkdir -p` semantics (`mkdirp` helper or recursive `mkdir` calls) for directory creation.
- Suggested new files: `snapshot.h`, `snapshot.c`.

---

### Task 3 — Change Detection (Comparison Engine)

**Goal:** Compare the live file system against a named snapshot and report what changed.

**Inputs:** A snapshot name and a directory to compare against.

**Expected output:**
```
[Added]
  new_file.txt
[Modified]
  main.c
[Deleted]
  old_file.txt
```

**Implementation notes:**
- Load `snapshot.meta` to get the baseline `FileInfo` list.
- Run `scan_directory` on the live directory to get the current state.
- Compare the two lists: match on path, then compare `size` and `modified_time`.
- A file present in the snapshot but absent live → **Deleted**.
- A file present live but absent in the snapshot → **Added**.
- A file present in both but with differing `size` or `mtime` → **Modified**.
- Optional advanced mode: verify with SHA-256 hashing (use `openssl/sha.h` or compute manually) to catch content changes that do not alter `mtime`.
- Suggested new files: `compare.h`, `compare.c`.

---

### Task 4 — Snapshot Manager (CLI)

**Goal:** Provide a command-line interface to list and delete snapshots.

**Supported commands:**
```
list_snapshots                  → prints all snapshots with creation time
delete_snapshot <name>          → removes the snapshot directory and meta file
create_snapshot <dir> <name>    → wraps Task 2
```

**Expected output:**
```
snap1 - created at 2026-04-30
snap2 - created at 2026-04-29
Snapshot snap1 deleted successfully.
```

**Implementation notes:**
- Snapshot creation time should be stored in `snapshot.meta` at write time (use `time(NULL)`).
- `list_snapshots` reads the top-level `/snapshots/` directory, opens each `snapshot.meta`, and prints the name and timestamp.
- `delete_snapshot` uses `rm -rf` equivalent logic (`nftw` with `FTW_DEPTH | FTW_PHYS` and `unlink`/`rmdir`).
- All commands should be reachable from `main.c` via `argv` parsing.
- Suggested new files: `manager.h`, `manager.c`.

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
# Current (Task 1 only)
gcc -o scanner main.c scanner.c -Wall -Wextra

# After adding new modules (example)
gcc -o snapshot main.c scanner.c snapshot.c compare.c manager.c restore.c -Wall -Wextra
```

No external dependencies beyond a standard POSIX C library are required for the core tasks. Optional SHA-256 hashing links against `-lssl` (`openssl/sha.h`).

---

## Suggested Development Order

1. **Fix the known issues** in `scanner.c` and `scanner.h` listed above before writing new code — they affect correctness of every downstream task.
2. Implement **Task 2** (snapshot creator) — all later tasks depend on `snapshot.meta`.
3. Implement **Task 3** (comparison engine) — validates that snapshots are correct.
4. Implement **Task 4** (CLI manager) — wires Tasks 2 & 3 together behind `argv`.
5. Implement **Task 5** (restore) — last because it is the most destructive operation and should be tested against known-good snapshots from Tasks 2–3.

---

## Contributing

- Follow the existing code style: POSIX C99, no dynamic allocation unless necessary.
- Each new module gets a `.h` / `.c` pair and a corresponding section in this README.
- Test against both shallow and deeply nested directory trees.
- All file operations must handle error returns — `perror` + `continue`/`return` is the established pattern in this codebase.
