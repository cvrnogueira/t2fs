#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
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
typedef struct t2fs_superbloco superblock;

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
superblock sblock;
BYTE buffer[SECTOR_SIZE];

/*-----------------------------------------------------------------------------
 * Implementation
-----------------------------------------------------------------------------*/
int initialize_superblock(void);

int is_success(code);

int is_error(code);

int is_error(int code) {
    return code == ERROR;
}

int is_success(int code) {
    return code == SUCCESS;
}

int initialize_superblock(void) {
    return SUCCESS;
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
    const char *szero;

    if (is_error(read_sector(0, buffer))) {
        return ERROR;
    }

    szero = (char *) buffer;

    strncpy(sblock.id, szero, 4);

    printf("superblock id -> %s\n", sblock.id);

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