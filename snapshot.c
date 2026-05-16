#include "../project/utils.c"
#include "../project/scanner.c"
#include "../project/copy.c"
#include "../project/storage.c"
#include "../project/compare.c"
#include "../project/restore.c"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: snapshot [command] [args]\n");
        printf("Commands:\n  scan [path]\n  create [snapshot_name] [path]\n  compare [snapshot_name]\n  restore [snapshot_name]\n");
        exit(0);
    }

    char *command = argv[1];

    if (strcmp(command, "scan") == 0)
    {
        char *path = (argc > 2) ? argv[2] : ".";
        run_scan(path);
    }
    else if (strcmp(command, "create") == 0)
    {
        if (argc < 3)
        {
            printf("Error: Provide snapshot name. (e.g., snapshot create v1 demo)\n");
            exit(0);
        }

        char *target = (argc > 3) ? argv[3] : ".";

        run_create(argv[2], target);
    }
    else if (strcmp(command, "compare") == 0)
    {
        if (argc < 3)
        {
            printf("Error: Provide snapshot to compare. (e.g., snapshot compare v1)\n");
            exit(0);
        }
        run_compare(argv[2]);
    }
    else if (strcmp(command, "restore") == 0)
    {
        if (argc < 3)
        {
            printf("Error: Provide snapshot to restore. (e.g., snapshot restore v1)\n");
            exit(0);
        }
        run_restore(argv[2]);
    }
    else
    {
        printf("Unknown command: %s\n", command);
    }

    exit(0);
}