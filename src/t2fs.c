#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "t2fs.h"
#include "apidisk.h"

#define FS_VERSION 0x7E22
#define FS_ID "T2FS"
#define MAX_OPENED_FILES 10
#define FREE_CLUSTER 0x00000000
#define INVALID_CLUSTER 0x00000001
#define BAD_SECTOR 0xFFFFFFFE
#define END_OF_FILE 0xFFFFFFFF

#define ERROR 1
#define SUCCESS 0

typedef struct t2fs_superbloco superblock;

superblock sblock;

/* Initialize the disk, copying the values of superblock, checking if the data is consistent.
Futhermore, allocate memory for currentPointer and set the value for currentDir for root (in sectors)*/

/*
 * Function: square_the_biggest
 * ----------------------------
 *   Initialize disks, copying the values of super block checking whether the data is consistent or not.
 *
 *   returns: the square of the larger of n1 and n2
 */
int initialize_superblock();

int is_success();

int is_error();

int is_error(int code) {
    return code == ERROR;
}

int is_success(int code) {
    return code == SUCCESS;
}

int initialize_superblock() {
    BYTE buffer[SECTOR_SIZE];

    const char *szero;

    int bootSector = read_sector(0, buffer);

    if (is_error(bootSector)) {
        return ERROR;
    }

    szero = (char *) buffer;

    strncpy(sblock.id, szero, 4);

    return SUCCESS;
}

int main() {
    initialize_superblock();

    printf("%s", sblock.id);
}