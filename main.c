#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "VirtualFS.h"

int main(int argc, char *argv[]) {
    int i;
    if (argc < 2) {
        printf("usage: %s <filename> [optional arguments]\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *vfs_name;
    const char *command;
    vfs_name = argv[1];
    command = argv[2];
    if(strcmp(command, "create") == 0) {
        VFS *vfs;
        vfs = createVFS(vfs_name, atoi(argv[3]));
        if (!vfs) {
            printf("Could not create VFS");
        } else {
            printf("VFS successfully created");
            closeVFS(vfs);
        }
    }
    else if (strcmp(command, "delete") == 0) {
        deleteVFS(vfs_name);
    }
    else if (strcmp(command, "list") == 0) {
        VFS *vfs;
        vfs = openVFS(vfs_name);
        if (!vfs) {
            printf("Could not open %s VFS.", vfs_name);
            return EXIT_FAILURE;
        }
        listVFS(vfs);
        closeVFS(vfs);
    }
    else if (strcmp(command, "dump") == 0) {
        VFS *vfs;
        vfs = openVFS(vfs_name);
        if (!vfs) {
            printf("Could not open %s VFS.", vfs_name);
            return EXIT_FAILURE;
        }
        dumpVFS(vfs);
        closeVFS(vfs);
    }
    else if (strcmp(command, "push") == 0) {
        VFS *vfs;
        vfs = openVFS(vfs_name);
        if (!vfs) {
            printf("Could not open %s VFS.", vfs_name);
            return EXIT_FAILURE;
        }
        i = copyTo(vfs, argv[3], argv[4]);
        if(i != 0) {
            printf("Could not copy the file to VFS.\n");
            closeVFS(vfs);
            return i;
        }
        printf("File copied successfully.\n");
        closeVFS(vfs);
    }
    else if (strcmp(command, "remove") == 0) {
        VFS *vfs;
        vfs = openVFS(vfs_name);
        if (!vfs) {
            printf("Could not open %s VFS.", vfs_name);
            return EXIT_FAILURE;
        }
        i = deleteFromVFS(vfs, argv[3]);
        if(i != 0) {
            printf("Could not remove the file from VFS.\n");
            closeVFS(vfs);
            return i;
        }
        printf("File removed successfully.\n");
        closeVFS(vfs);
    }
    else if (strcmp(command, "pull") == 0) {
        VFS *vfs;
        vfs = openVFS(vfs_name);
        if (!vfs) {
            printf("Could not open %s VFS.", vfs_name);
            return EXIT_FAILURE;
        }
        i = copyFrom(vfs, argv[3], argv[4]);
        if(i != 0) {
            printf("Could not copy the file from VFS.\n");
            closeVFS(vfs);
            return i;
        }
        printf("File copied successfully.\n");
        closeVFS(vfs);
    }
    return EXIT_SUCCESS;
}
