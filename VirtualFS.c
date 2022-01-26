#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "VirtualFS.h"

#define WRONG_NAME_ERROR (-1)
#define FILE_OPENING_ERROR (-2)
#define NOT_ENOUGH_SPACE_ERROR (-3)
#define FILE_NOT_FOUND_ERROR (-4)

VFS *openVFS(const char *file_name) {
    VFS *vfs;                                               //pointer to Virtual Files System
    FILE *f;                                                //handle to our VFS file
    unsigned int num_nodes;                                 //number of nodes in our VFS
    Inode *nodes;                                           //pointer to all the nodes in VFS
    size_t size;                                            //size of provided potential VFS file
    Superblock sb;                                          //first block of our VFS that contains its size

    f = fopen(file_name, "r+b");
    if(!f) {
        return NULL;
    }                                                       //file doesn't exists because it could not be opened for reading

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fread(&sb, sizeof(Superblock), 1, f) <= 0) {
        fclose(f);
        return NULL;
    }
    if (size != sb.size) {
        fclose(f);
        return NULL;
    }                                                       //checking if size written in the superblock is the same as size of provided file

    num_nodes = (size - sizeof(Superblock)) / (sizeof(Inode) + BLOCK_SIZE);
    nodes = malloc(sizeof(Inode) * num_nodes);
    if (fread(nodes, sizeof(Inode), num_nodes, f) <= 0) {
        fclose(f);
        return NULL;
    }

    vfs = malloc(sizeof(VFS));
    vfs->fhandle = f;
    vfs->num_nodes = num_nodes;
    vfs->inodes = nodes;

    return vfs;
}

void closeVFS(VFS *vfs) {
    fseek(vfs->fhandle, sizeof(Superblock), SEEK_SET);
    fwrite(vfs->inodes, sizeof(Inode), vfs->num_nodes, vfs->fhandle);

    fclose(vfs->fhandle);
    free(vfs->inodes);
    free(vfs);
    vfs = NULL;
}

VFS *createVFS(const char *file_name, size_t size) {
    VFS *vfs;                                               //pointer to Virtual Files System
    FILE *f;                                                //handle to our VFS file
    Superblock sb;                                          //first block of our VFS that contains its size
    unsigned int num_nodes;                                 //number of nodes in our VFS
    Inode *nodes;                                           //pointer to all the nodes in VFS
    char zero_buff[128];                                    //buffer used to fill VFS file with zeros
    size_t bytes_remaining;                                 //remaining bytes in our VFS file
    size_t bytes_to_write;                                  //bytes to write to our VFS file
    int i;
    unsigned long w;

    f = fopen(file_name, "wb");
    if(!f) {
        return NULL;
    }                                                       //checking if VFS file can be created

    memset(zero_buff, 0, sizeof(zero_buff));
    bytes_remaining = size * 1024;
    while (bytes_remaining > 0) {
        bytes_to_write = sizeof(zero_buff);
        if (bytes_to_write > bytes_remaining) {
            bytes_to_write = bytes_remaining;
        }
        fwrite(zero_buff, 1, bytes_to_write, f);
        bytes_remaining -= bytes_to_write;
    }                                                       //filling VFS binary file with zeros

    fseek(f, 0, SEEK_SET);                                  //setting superblock as first bytes of VFS
    sb.size = size * 1024;
    w = fwrite(&sb, sizeof(Superblock), 1, f);
    if (w == 0) {
        return NULL;
    }

    num_nodes = (size * 1024 - sizeof(Superblock)) % (sizeof(Inode) + BLOCK_SIZE) == 0 ? (size * 1024 - sizeof(Superblock)) / (sizeof(Inode) + BLOCK_SIZE) : (size * 1024 - sizeof(Superblock)) / (sizeof(Inode) + BLOCK_SIZE) + 1;     //counting number of inodes
    nodes = malloc(sizeof(Inode) * num_nodes);
    for (i = 0; i < num_nodes; i++) {
        nodes[i].flags = 0;
    }
    vfs = malloc(sizeof(VFS));
    vfs->fhandle = f;
    vfs->num_nodes = num_nodes;
    vfs->inodes = nodes;

    return vfs;
}

void deleteVFS(const char *file_name) {
    int error;
    error = unlink(file_name);
    if (error == 0) {
        printf("VFS deleted successfully");
    } else {
        printf("An error occured when trying to delete VFS");
    }
}

void listVFS(VFS *vfs) {
    int i;
    for(i = 0; i < vfs->num_nodes; i++)
    {
        if((vfs->inodes[i].flags & IN_USE) && (vfs->inodes[i].flags & IS_START))
        {
            printf("File: %s @%d\n", vfs->inodes[i].name, i);
        }
    }
}

void dumpVFS(VFS *vfs) {
    int available_nodes;
    int i;

    printf("==============\n");
    printf("====iNodes====\n");
    printf("==============\n");
    for (i = 0; i < vfs->num_nodes; i++) {
        printf("> ID: %d\n", i);
        printf("> Flags: %d\n", vfs->inodes[i].flags);
        printf("> Name: %s\n", vfs->inodes[i].name);
        printf("> Data size: %d\n", vfs->inodes[i].size);
        printf("> Next Node: %d\n", vfs->inodes[i].next_node);
        printf("\n\n");
    }
    printf("==================\n");
    printf("====Memory map====\n");
    printf("==================\n");
    available_nodes = 0;
    for(i = 0; i < vfs->num_nodes; i++)
    {
        if(!(vfs->inodes[i].flags & IN_USE))
            available_nodes++;

        printf("%c", (vfs->inodes[i].flags & IN_USE ? (vfs->inodes[i].flags & IS_START ? '$' : (vfs->inodes[i].next_node == -1 ? '#' : '>')) : '.'));
    }

    printf("\n==================\n");
    printf("Summary: %d nodes out of %d available.\n", available_nodes, vfs->num_nodes);
}

int copyTo(VFS *vfs, const char *src_name, const char *dest_name) {
    FILE *fsource;
    size_t source_size;
    int req_nodes;
    int *nodes_queue;
    int node;
    int i;
    char buff[BLOCK_SIZE];

    if (strlen(dest_name) == 0) {
        return WRONG_NAME_ERROR;
    }                                                    //checking if destination file name is empty
    for(i = 0; i < vfs->num_nodes; i++)
    {
        if(vfs->inodes[i].flags & IN_USE && vfs->inodes[i].flags & IS_START && strncmp(vfs->inodes[i].name, dest_name, MAX_NAME) == 0) {
            return WRONG_NAME_ERROR;
        }
    }                                                   //checking if file with provided name exists

    fsource = fopen(src_name, "r+b");
    if (!fsource) {
        return FILE_OPENING_ERROR;
    }

    fseek(fsource, 0, SEEK_END);
    source_size = ftell(fsource);
    fseek(fsource, 0, SEEK_SET);

    req_nodes = (source_size % BLOCK_SIZE) == 0 ? (int)(source_size / BLOCK_SIZE) : ((int)(source_size / BLOCK_SIZE) + 1);
    nodes_queue = malloc(sizeof(Inode) * req_nodes);

    node = 0;
    for(i = 0; i < vfs->num_nodes; i++) {
        if ((vfs->inodes[i].flags & IN_USE) == 0) {
            nodes_queue[node++] = i;
        }
        if (node == req_nodes) {
            break;
        }
    }                                                  //nodes_queue stores indexes of nodes that will hold the file

    if (node < req_nodes) {
        free(nodes_queue);
        fclose(fsource);
        return NOT_ENOUGH_SPACE_ERROR;
    }                                                       //checking if there's enough space for the file

    for (i = 0; i < req_nodes; i++) {
        vfs->inodes[nodes_queue[i]].flags = IN_USE;
        vfs->inodes[nodes_queue[i]].size = fread(buff, 1, BLOCK_SIZE, fsource);
        fseek(vfs->fhandle, sizeof(Superblock) + sizeof(Inode) * vfs->num_nodes + BLOCK_SIZE * nodes_queue[i], SEEK_SET);
        fwrite(buff, 1, vfs->inodes[i].size, vfs->fhandle);
        vfs->inodes[nodes_queue[i]].next_node = nodes_queue[i + 1];
    }
    vfs->inodes[nodes_queue[0]].flags |= IS_START;
    strncpy(vfs->inodes[nodes_queue[0]].name, dest_name, MAX_NAME);
    vfs->inodes[nodes_queue[req_nodes - 1]].next_node = -1;

    free(nodes_queue);
    fclose(fsource);

    return EXIT_SUCCESS;
}

int deleteFromVFS(VFS *vfs, const char *file_name) {
    int first_node;
    int old_node;
    int i;
    char buff[BLOCK_SIZE];
    bool node_found = false;
    memset(buff, 0, sizeof(buff));

    for (i = 0; i < vfs->num_nodes; i++) {
        if(vfs->inodes[i].flags & IN_USE && vfs->inodes[i].flags & IS_START && strncmp(vfs->inodes[i].name, file_name, MAX_NAME) == 0) {
            first_node = i;
            node_found = true;
            break;
        }
    }                                           //index of first node of file
    if (!node_found) {
        return FILE_NOT_FOUND_ERROR;
    } else {
        vfs->inodes[first_node].flags &= ~IS_START;
        strncpy(vfs->inodes[first_node].name, "", MAX_NAME);
        while (first_node != -1) {
            vfs->inodes[first_node].flags &= ~IN_USE;
            vfs->inodes[first_node].size = 0;
            fseek(vfs->fhandle, sizeof(Superblock) + sizeof(Inode) * vfs->num_nodes + BLOCK_SIZE * first_node, SEEK_SET);
            fwrite(buff, 1, BLOCK_SIZE, vfs->fhandle);
            memcpy(&old_node, &first_node, sizeof(int));
            first_node = vfs->inodes[first_node].next_node;
            vfs->inodes[old_node].next_node = 0;
        }
    }
    return EXIT_SUCCESS;
}

int copyFrom(VFS *vfs, const char *src_name, const char *dest_name) {
    FILE *fdest;
    int first_node;
    int i;
    char buff[BLOCK_SIZE];

    if (strlen(dest_name) == 0) {
        return WRONG_NAME_ERROR;
    }                                                    //checking if destination file name is empty

    fdest = fopen(dest_name, "w+b");
    if (!fdest) {
        return FILE_OPENING_ERROR;
    }


    first_node = -1;
    for(i = 0; i < vfs->num_nodes; i++)
    {
        if(vfs->inodes[i].flags & IN_USE && vfs->inodes[i].flags & IS_START && strncmp(vfs->inodes[i].name, src_name, MAX_NAME) == 0) {
            first_node = i;
            break;
        }
    }

    while(first_node != -1) {
        fseek(vfs->fhandle, sizeof(Superblock) + sizeof(Inode) * vfs->num_nodes + BLOCK_SIZE * first_node, SEEK_SET);
        if(fread(buff, 1, BLOCK_SIZE, vfs->fhandle) == 0) {
            fclose(fdest);
            return FILE_OPENING_ERROR;
        }
        if(fwrite(buff, 1, BLOCK_SIZE, fdest) == 0) {
            fclose(fdest);
            return FILE_OPENING_ERROR;
        }
        first_node = vfs->inodes[first_node].next_node;
    }
    fclose(fdest);

    return EXIT_SUCCESS;
}
