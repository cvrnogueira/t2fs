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
#define FAT_ENTRY_SIZE 4

#define ERROR 1
#define SUCCESS 0

/*-----------------------------------------------------------------------------
 * structs / typedefs
-----------------------------------------------------------------------------*/
typedef struct t2fs_superbloco Superblock;

typedef struct {
  char *head;
  char *tail;
} Path;

/*-----------------------------------------------------------------------------
 * variables
-----------------------------------------------------------------------------*/
Superblock superblock;

DWORD curr_dir;

BYTE buffer[SECTOR_SIZE];

/*-----------------------------------------------------------------------------
 * function definition
-----------------------------------------------------------------------------*/
static void initialize(void) __attribute__((constructor));

Path path_from_name(char *name);
DWORD first_fit(void);
int initialize_curr_dir(Superblock *superblock);
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
 * implementation
-----------------------------------------------------------------------------*/
/**
 * splits path name into head and tail parts.
 *
 * eg.: path consisting of the following name will be split as:
 *  
 *  name -> /home/aluno/sisop/t2fs 
 *  
 *  tail -> /home/aluno/sisop 
 *  head -> t2fs
 * 
 * returns  - Path struct with head and tail.
**/
Path path_from_name(char *name) {
    // calculate name length
    const name_len = strlen(name) + 1;

    // allocate head, tail and aux last_token character
    // arrays that will fill path struct later on
    char head[name_len]; 
    char tail[name_len]; 
    char last_token[name_len];

    // empty string arrays
    head[0] = 0;
    tail[0] = 0;
    last_token[0] = 0;

    // create an auxiliary array from name
    // acting as a buffer to strtok without
    // modifying external name variable passed
    // as reference to this function
    char aux_token[strlen(name) + 1];
    strcpy(aux_token, name);

    // tokenize name splitting by "/"
    char *token = strtok(aux_token, "/");

    // iterate through tokenizer concating current 
    // head to tail until theres no slash character
    // found in name path anymore
    // eg.: 
    // name -> /home/aluno/sisop/t2fs 
    // ite(1): tail -> /home | head -> aluno
    // ite(1): tail -> /home/aluno | head -> sisop
    // ite(1): tail -> /home/aluno/sisop | head -> t2fs
    do {
        
        if (token != NULL && strlen(head) >= 1) {
            strcat(tail, head);
        }

        strcpy(head, token);

        token = strtok(NULL, "/");

        if (token != NULL) {
            strcat(tail, "/");
        }

    } while (token != NULL);

    // create path struct
    Path path;

    // fill with tail and head generated
    // from tokenizer steps
    path.tail = tail;
    path.head = head;
    
    return path;
}

/**
 * finds first free entry in FAT.
 * 
 * returns  - index of entry since first sector entry within FAT.
 * on error - returns -1 if cant read fat partition or theres no free entry.
**/
DWORD first_fit(void) {
    int sector = 0;

    // loop through fat sectors until end of fat (data sector is reached)  
    for (sector = superblock.pFATSectorStart; sector < superblock.DataSectorStart; sector++) {
        int entry = 0;

        // a sector has 256 bytes in total and will be read into
        // buffer global var
        const int can_read = read_sector(sector, buffer);

        // if we cannot read this sector for some reason
        // something bad happened to disk and we should return error
        if (can_read == ERROR) return ERROR;

        // a sector contains 64 entries made up of 4 bytes each
        while (entry < SECTOR_SIZE) {
            // read current entry value
            const DWORD entry_val = *((DWORD *) buffer + entry);

            // check if current entry points to a free cluster (represented by value 0x00000000)
            if (entry_val == FREE_CLUSTER) {
                // calculate initial point for each sector
                // decrementing since sectors are zero based
                // and superblock fat start is one based
                // eg.: sector = 1
                // (1 - 1) * 256 = 0
                // eg.: sector = 2
                // (2 - 1) * 256 = 256
                const int cur_sector = (sector - 1) * SECTOR_SIZE;
                
                // since entry are made up by 4 bytes we must
                // divide it by fat entry size (4 bytes) to calculate
                // current entry index
                const int cur_entry = entry / FAT_ENTRY_SIZE;

                // return entry index which is made by sector and entry summed up
                return cur_sector + cur_entry;
            }

            // increment entry by one
            entry += FAT_ENTRY_SIZE;
        }
    }

    // unable to find a free cluster in fat
    return ERROR;
}

int initialize_curr_dir(Superblock *superblock) {
    curr_dir = superblock->DataSectorStart + superblock->RootDirCluster * superblock->SectorsPerCluster;
    return SUCCESS;
}

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

    initialize_curr_dir(&superblock);
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

    char path[strlen(pathname) + 1], name[MAX_FILE_NAME_SIZE];
    extractPath("/home/pablo/loves/cat", path, name);

    printf("out path %s\n", path);
    printf("out name %s\n", name);

    Path pp;

    pp = path_from_name("/home/pablo/loves/cat");

    printf("pp tail %s\n", pp.tail);
    printf("pp head %s\n", pp.head);

    DWORD parent;

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