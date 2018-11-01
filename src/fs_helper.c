#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "../include/fs_helper.h"
#include "../include/t2fs.h"
#include "../include/apidisk.h"

/**
 * Called by gcc attributes before main execution and responsible for
 * global vars initialization
 * 
 * on error - sigkill.
**/
static void initialize(void) __attribute__((constructor));
static void initialize(void) {
    // initialize superblock and store function exit code
    int is_superblock_init = initialize_superblock();

    // if cannot initialize superblock then kill program by sending
    // a sigkill
    if (is_error(is_superblock_init)) {
        psignal(SIGTERM, "cannot initialize superblock from sector zero");
        raise(SIGTERM);
    }

    // initialize current directory poiting to data dir after root
    // based on superblock structure
    initialize_curr_dir(&superblock);
}

/**
 * Initialize curr_dir position to data sector after root sectors.
**/
int initialize_curr_dir(Superblock *superblock) {
    curr_dir = superblock->DataSectorStart + superblock->RootDirCluster * superblock->SectorsPerCluster;
    return SUCCESS;
}

/**
 * Read superblock from sector zero
 * 
 * on error - return -1 if cant read superblock from sector zero
**/
int initialize_superblock(void) {
    // read first logical sector from disk
    int can_read = read_sector(0, buffer);

    // something bad happened, disk may be corrupted
    if (is_error(can_read)) {
        return ERROR;
    }

    // aux buffer to extract fs id
    const char *buffer_char;
    buffer_char = (char *) buffer;

    // fill fs id
    strncpy(superblock.id, buffer_char, 4);

    // test fs id
    if(strncmp(superblock.id, FS_ID, 4) != SUCCESS) {
        return ERROR;
    }

    // fill fs version
    superblock.version = *((WORD *) (buffer + 4));

    // test fs version
    if(superblock.version != FS_VERSION) {
        return ERROR;
    }

    // fill fs parameters accordingly to specs
    superblock.superblockSize    = *((WORD *)  (buffer + 6));
    superblock.DiskSize          = *((DWORD *) (buffer + 8));
    superblock.NofSectors        = *((DWORD *) (buffer + 12));
    superblock.SectorsPerCluster = *((DWORD *) (buffer + 16));
    superblock.pFATSectorStart   = *((DWORD *) (buffer + 20));
    superblock.RootDirCluster    = *((DWORD *) (buffer + 24));
    superblock.DataSectorStart   = *((DWORD *) (buffer + 28));

    return SUCCESS;
}

/**
 * Splits path name into head and tail parts storing
 * it on result parameter.
 *
 * eg.: path consisting of the following name will be split as:
 *  
 *  name -> /home/aluno/sisop/t2fs 
 *  
 *  tail -> /home/aluno/sisop 
 *  head -> t2fs
**/
void path_from_name(char *name, Path *result) {
    // calculate name length
    const int name_len = strlen(name);

    // allocate head, tail character
    // arrays that will fill path struct later on
    char head[name_len]; 
    char tail[name_len];

    // empty string arrays
    head[0] = 0;
    tail[0] = 0;

    // extract first character from name parameter
    char fst_char_raw = *((BYTE*) name);

    // check if first character of name parameter is a dot
    int starts_with_dot = fst_char_raw == '.';

    // remove dot from path if name parameter
    // starts with it
    char *sanitized_name = starts_with_dot ? name + 1 : name;

    // create an auxiliary array from name
    // acting as a buffer to strtok without
    // modifying external name variable passed
    // as reference to this function
    char aux_token[strlen(sanitized_name)];
    strcpy(aux_token, sanitized_name);

    // tokenize name splitting by "/"
    char *token = strtok(aux_token, "/");

    // check if token is equal to name parameter
    int is_equal = token != NULL && strcmp(token, sanitized_name) == 0;

    // ignore initial slash character from sanitized name
    // and check if token is equal to it
    int is_eq_ignore_slash = token != NULL && strcmp(token, (sanitized_name + 1)) == 0;

    // extract first character from sanitized name
    char fst_char_san = *((BYTE*) sanitized_name);

    // check if first character of sanitized name parameter 
    // is a slash
    int starts_with_slash = fst_char_san == '/';

    // check if name parameter is relative path
    // eg.:
    // curr_dir -> /home/aluno
    // name -> t2fs or /t2fs or ./t2fs
    int is_relative =  starts_with_slash && is_eq_ignore_slash || is_equal;

    // if path is relative then set tail to ./ and head to path name
    // eg.:
    // curr_dir -> /home/aluno
    // name -> t2fs or /t2fs or ./t2fs
    // tail -> ./ | head -> t2fs
    if (is_relative) {
        strncpy(head, token, strlen(token) + 1);
        strncat(tail, "./", 2);

    } else {
        // iterate through tokenizer concating current 
        // head to tail until theres no slash character
        // found in name path anymore
        // eg.: 
        // name -> /home/aluno/sisop/t2fs 
        // ite(1): tail -> /home | head -> aluno
        // ite(2): tail -> /home/aluno | head -> sisop
        // ite(3): tail -> /home/aluno/sisop | head -> t2fs
        do {
            
            if (token != NULL && strlen(head) >= 1) {
                strncat(tail, head, strlen(head) + 1);
            }

            strncpy(head, token, strlen(token) + 1);

            token = strtok(NULL, "/");

            if (token != NULL) {
                strncat(tail, "/", 1);
            }

        } while (token != NULL);
    }

    // fill with tail and head generated
    // from tokenizer steps
    memcpy(result->head, head, strlen(head) + 1);
    memcpy(result->tail, tail, strlen(tail) + 1);
}

/**
 * Physical cluster size calculated by sector per cluster from superblock.
 * 
 * returns  - physical cluster size.
**/
DWORD phys_cluster_size(void) {
    return SECTOR_SIZE * superblock.SectorsPerCluster;
}

/**
 * Convert logical FAT sector entry to physical sector entry.
 * 
 * returns  - physical sector entry in FAT.
**/
int fat_log_to_phys(int lsector) {
    return (lsector -1) * SECTOR_SIZE;
}

/**
 * Convert phyisical FAT sector entry to logical sector entry.
 * 
 * returns  - logical sector entry in FAT.
**/
int fat_phys_to_log(int psector) {
    return (int)((double) psector / SECTOR_SIZE);
}

/**
 * Finds first free physical entry in FAT.
 * 
 * returns  - physical entry index since first sector in FAT.
 * on error - returns -1 if cant read fat partition or theres no free entry.
**/
DWORD phys_fat_first_fit(void) {
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
            // increment buffer by entry
            const DWORD entry_pos = buffer + entry;

            // read current entry value
            const DWORD entry_val = *(DWORD *) entry_pos;

            // check if current entry points to a free cluster (represented by value 0x00000000)
            if (entry_val == FREE_CLUSTER) {
                // calculate initial point for each sector
                // decrementing since sectors are zero based
                // and superblock fat start is one based
                // eg.: sector = 1
                // (1 - 1) * 256 = 0
                // eg.: sector = 2
                // (2 - 1) * 256 = 256
                const int cur_sector = fat_log_to_phys(sector);
                
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

int is_error(int code) {
    return code == ERROR;
}

int is_success(int code) {
    return code == SUCCESS;
}

void read_cluster(int cluster, char *result) {
    int starting_sector = superblock.DataSectorStart + cluster * superblock.SectorsPerCluster;
    int sector_index;
    for(sector_index = 0; sector_index < superblock.SectorsPerCluster; sector_index++) {
        read_sector(starting_sector + sector_index, &result[sector_index * SECTOR_SIZE]);
    }
}

void print_descriptor(struct t2fs_record descriptor, int tab) {
    int tabix;
    for (tabix = 0; tabix < tab; tabix++)
         printf("\t");
    printf("%s = %u bytes (%u clusters)\n", descriptor.name, descriptor.bytesFileSize, descriptor.clustersFileSize);
}

void print_dir(int cluster, int tab) {
   int cluster_size = SECTOR_SIZE * superblock.SectorsPerCluster;
    char result[cluster_size];
    struct t2fs_record descriptor;  
    int index; // Descriptor index

    read_cluster(cluster, result);

    // 64 = tamanho da estrutura t2fs_record
    for(index = 0; index < cluster_size / 64; index++) {
        memcpy(&descriptor, &result[index * 64], sizeof(descriptor));
	if (descriptor.TypeVal != TYPEVAL_INVALIDO)
            print_descriptor(descriptor, tab);
        if (descriptor.TypeVal == TYPEVAL_DIRETORIO && index > 1)
            print_dir(descriptor.firstCluster, tab+1);
    }
}

void print_disk() {
    printf("\nDISK\n");
    print_dir(superblock.RootDirCluster, 0);
}

void print_superblock() {
    printf("\nSUPERBLOCK\n");
    printf("- id -> %.4s\n", superblock.id);
    printf("- superblockSize -> %d\n", superblock.superblockSize);
    printf("- DiskSize -> %d\n", superblock.DiskSize);
    printf("- NofSectors -> %d\n", superblock.NofSectors);
    printf("- SectorsPerCluster -> %d\n", superblock.SectorsPerCluster);
    printf("- pFATSectorStart -> %d\n", superblock.pFATSectorStart);
    printf("- RootDirCluster -> %d\n", superblock.RootDirCluster);
    printf("- DataSectorStart -> %d\n", superblock.DataSectorStart);
}
