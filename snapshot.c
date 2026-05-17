#include "kernel/types.h"
#include "user/user.h"

// Include all modular component interfaces
#include "scanner.h"
#include "copy.h"
#include "storage.h"
#include "compare.h"
#include "restore.h"
#include "info.h"
#include "delete.h"

int main(int argc, char *argv[]) {
  if(argc < 2) {
    printf("Usage: snapshot [command] [args]\n");
    printf("Commands:\n");
    printf("  scan [path]\n");
    printf("  create [name] [target_dir]\n");
    printf("  compare [name]\n");
    printf("  restore [name]\n");
    printf("  info [name]\n");
    printf("  delete [name]\n");
    exit(0);
  }

  char *command = argv[1];

  if(strcmp(command, "scan") == 0) {
    char *path = (argc > 2) ? argv[2] : ".";
    run_scan(path);
  } 
  else if(strcmp(command, "create") == 0) {
    if(argc < 3) {
      printf("Error: Provide snapshot name. (e.g., snapshot create v1 .)\n");
      exit(0);
    }
    char *target = (argc > 3) ? argv[3] : ".";
    run_create(argv[2], target);
  } 
  else if(strcmp(command, "compare") == 0) {
    if(argc < 3) {
      printf("Error: Provide snapshot name to compare.\n");
      exit(0);
    }
    run_compare(argv[2]);
  } 
  else if(strcmp(command, "restore") == 0) {
    if(argc < 3) {
      printf("Error: Provide snapshot name to restore.\n");
      exit(0);
    }
    run_restore(argv[2]);
  } 
  else if(strcmp(command, "info") == 0) {
    if(argc < 3) {
      printf("Error: Provide snapshot name to view.\n");
      exit(0);
    }
    run_info(argv[2]);
  } 
  else if(strcmp(command, "delete") == 0) {
    if(argc < 3) {
      printf("Error: Provide snapshot name to purge.\n");
      exit(0);
    }
    run_delete(argv[2]);
  } 
  else {
    printf("Unknown command: %s\n", command);
  }

  exit(0);
}