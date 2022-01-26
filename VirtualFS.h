#pragma once
#ifndef LAB6_VIRTUALFS_H
#define LAB6_VIRTUALFS_H

#define MAX_NAME 32                             //maximal length of a file name
#define BLOCK_SIZE 4096                         //each block has a size of 4KB = 4096b
#define IN_USE 0b01                             //is node free of occupied
#define IS_START 0b10                           //is the node a beginning of a file or not

typedef struct Superblock {
    unsigned int size;                          //size of the VFS
} Superblock;

typedef struct Inode {
    unsigned int flags;                         //a flag wether an inode is occupied or is it a beginning of a file
    char name[MAX_NAME];                        //name of the file in VFS
    unsigned int size;                          //size of the file
    int next_node;                              //index of the next node or -1 if it is the last node
} Inode;

typedef struct VFS {
    FILE *fhandle;                              //pointer to a file with our VFS
    unsigned int num_nodes;                     //number of nodes in VFS
    Inode *inodes;                              //pointer to all the nodes
} VFS;

VFS *createVFS(const char *file_name, size_t size);
VFS *openVFS(const char *file_name);
void closeVFS(VFS *vfs);
void deleteVFS(const char *file_name);

int copyTo(VFS *vfs, const char *src_name, const char *dest_name);
int copyFrom(VFS *vfs, const char *src_name, const char *dest_name);
void listVFS(VFS *vfs);
void dumpVFS(VFS *vfs);
int deleteFromVFS(VFS *vfs, const char *file_name);

#endif //LAB6_VIRTUALFS_H
