#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "../include/fs_helper.h"
#include "../include/t2fs.h"
#include "../include/apidisk.h"

static void initialize(void) __attribute__((constructor));

/**
 * Splits path name into head and tail parts.
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
    const int name_len = strlen(name) + 1;

    // allocate head, tail character
    // arrays that will fill path struct later on
    char head[name_len]; 
    char tail[name_len]; 

    // empty string arrays
    head[0] = 0;
    tail[0] = 0;

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
