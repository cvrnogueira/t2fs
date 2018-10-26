#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "apidisk.h"
#include "t2fs.h"

/*-----------------------------------------------------------------------------
 * Constants
-----------------------------------------------------------------------------*/
#define FS_VERSION 0x7E22
#define FS_ID "T2FS"
#define MAX_OPENED_FILES 10
#define FREE_CLUSTER 0x00000000
#define INVALID_CLUSTER 0x00000001
#define BAD_SECTOR 0xFFFFFFFE
#define END_OF_FILE 0xFFFFFFFF

#define ERROR 1
#define SUCCESS 0

/*-----------------------------------------------------------------------------
 * Definition
-----------------------------------------------------------------------------*/
typedef struct t2fs_superbloco Superblock;

static void initialize(void) __attribute__((constructor));

int initialize_superblock(void);
int is_success(int code);
int is_error(int code);
int identify2 (char *name, int size);
FILE2 create2 (char *filename);
int delete2 (char *filename);
FILE2 open2 (char *filename);
int close2 (FILE2 handle);
int read2 (FILE2 handle, char *buffer, int size);
int write2 (FILE2 handle, char *buffer, int size);
int truncate2 (FILE2 handle);
int seek2 (FILE2 handle, DWORD offset);
int mkdir2 (char *pathname);
int rmdir2 (char *pathname);
int chdir2 (char *pathname);
int getcwd2 (char *pathname, int size);
DIR2 opendir2 (char *pathname);
int readdir2 (DIR2 handle, DIRENT2 *dentry);
int closedir2 (DIR2 handle);
int ln2(char *linkname, char *filename);

/*-----------------------------------------------------------------------------
 * Variables
-----------------------------------------------------------------------------*/
Superblock superblock;

BYTE buffer[SECTOR_SIZE];

int initialized = 0;

/*-----------------------------------------------------------------------------
 * Implementation
-----------------------------------------------------------------------------*/
int initialize_superblock(void) {
    int can_read = read_sector(0, buffer);

    if (is_error(can_read)) {
        return ERROR;
    }

    const char *buffer_char;

    buffer_char = (char *) buffer;

    strncpy(superblock.id, buffer_char, 4);

    if(strncmp(superblock.id, FS_ID, 4) != SUCCESS) {
        return ERROR;
    }

    superblock.version = *((WORD *) (buffer + 4));

    if(superblock.version != FS_VERSION) {
        return ERROR;
    }

    superblock.superblockSize    = *((WORD *)  (buffer + 6));
    superblock.DiskSize          = *((DWORD *) (buffer + 8));
    superblock.NofSectors        = *((DWORD *) (buffer + 12));
    superblock.SectorsPerCluster = *((DWORD *) (buffer + 16));
    superblock.pFATSectorStart   = *((DWORD *) (buffer + 20));
    superblock.RootDirCluster    = *((DWORD *) (buffer + 24));
    superblock.DataSectorStart   = *((DWORD *) (buffer + 28));

    return SUCCESS;
}

static void initialize(void) {
    int is_superblock_init = initialize_superblock();

    if (is_error(is_superblock_init)) {
        psignal(SIGTERM, "cannot initialize superblock from sector zero");
        raise(SIGTERM);
    }
}

int is_error(int code) {
    return code == ERROR;
}

int is_success(int code) {
    return code == SUCCESS;
}

int identify2 (char *name, int size) {
    return SUCCESS;
}

FILE2 create2 (char *filename) {
    return NULL;
}

int delete2 (char *filename) {
    return SUCCESS;
}


FILE2 open2 (char *filename) {
    return NULL;
}

int close2 (FILE2 handle) {
    return SUCCESS;
}

int read2 (FILE2 handle, char *buffer, int size) {
    return SUCCESS;
}

int write2 (FILE2 handle, char *buffer, int size) {
    return SUCCESS;
}

int truncate2 (FILE2 handle) {
    return SUCCESS;
}

int seek2 (FILE2 handle, DWORD offset) {
    return SUCCESS;
}

int mkdir2 (char *pathname) {
    printf("superblock id -> %s\n", superblock.id);
    printf("superblock superblockSize -> %d\n", superblock.superblockSize);
    printf("superblock DiskSize -> %d\n", superblock.DiskSize);
    printf("superblock NofSectors -> %d\n", superblock.NofSectors);
    printf("superblock SectorsPerCluster -> %d\n", superblock.SectorsPerCluster);
    printf("superblock pFATSectorStart -> %d\n", superblock.pFATSectorStart);
    printf("superblock RootDirCluster -> %d\n", superblock.RootDirCluster);
    printf("superblock DataSectorStart -> %d\n", superblock.DataSectorStart);

    return SUCCESS;
}

int rmdir2 (char *pathname) {
    return SUCCESS;
}

int chdir2 (char *pathname) {
    return SUCCESS;
}

int getcwd2 (char *pathname, int size) {
    return SUCCESS;
}

DIR2 opendir2 (char *pathname) {
    return NULL;
}


int readdir2 (DIR2 handle, DIRENT2 *dentry) {
    return SUCCESS;
}

int closedir2 (DIR2 handle) {
    return SUCCESS;
}

int ln2(char *linkname, char *filename) {
    return SUCCESS;
}