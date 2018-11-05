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
    if (is_superblock_init == ERROR) {
        psignal(SIGTERM, "cannot initialize superblock from sector zero");
        raise(SIGTERM);
    }

    // initialize current directory poiting to data dir after root
    // based on superblock structure
    initialize_curr_dir(&superblock);

    // sets the opened file counter to 0
    num_opened_files = 0;
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
    if (can_read != SUCCESS) return ERROR;

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
 * Calculates logical data cluster based on current directory pointer.
 *
 * returns - logical data cluster based on current directory.
**/
DWORD curr_data_cluster(void) {
    return (curr_dir - superblock.DataSectorStart) / superblock.SectorsPerCluster;
}

/**
 * Converts a logical cluster number to sector number in data section.
 *
 * returns - sector number.
**/
DWORD cluster_to_log_sector(DWORD cluster) {
    return superblock.DataSectorStart + cluster * superblock.SectorsPerCluster;
}

/**
 * Number of records per sector.
 *
 * returns - number of records per sector.
**/
int records_per_sector(void) {
    return SECTOR_SIZE / RECORD_SIZE;
}

/**
 * Lookup record descriptor by name in cluster.
 *
 * param cluster - logical cluster number
 * param name    - record name that this function will try to match
 * param record  - if matched then this variable will store record found during lookup
 *
 * returns - TRUE if found FALSE otherwise.
**/
int lookup_descriptor_by_name(DWORD cluster, char *name, Record *record) {
    // convert cluster to sector
    int sector = cluster_to_log_sector(cluster);

    // calculate number of records that fits in sector
    int nr_of_records = records_per_sector();

    // max number of sectors from current sector to reach
    // clusters end boundary
    // since a cluster is usually formated as four sectors we
    // add SectorsPerCluster variable to our current sector variable
    // to calculate its cluster boundary
    int cluster_boundary = sector + superblock.SectorsPerCluster;

    // while in boundary
    while(sector <= cluster_boundary) {
        // record counter
        int i = 0;

        // read our cluster sectors
        read_sector(sector, buffer);

        // loop through records of current sector
        while(i < nr_of_records) {
            Record desc;

            // read our i(th) record from current sector
            memcpy(&desc, buffer + (RECORD_SIZE * i), RECORD_SIZE);

            // if record name equals to parameter name then we found our record
            // also make sure this record is valid checking its typeval 
            // typeval_invalido means that this record is empty
            if (strcmp(desc.name, name) == 0 && desc.TypeVal != TYPEVAL_INVALIDO) {
                
                // store record and return true since we found it
                memcpy(record, &desc, RECORD_SIZE);
                return TRUE;
            }

            i++;
        }

        sector++;
    }

    // unable to find record in cluster
    return FALSE;
}

/**
 * Lookup a record returning a contiguous logical position 
 * of found record accumulating positions from previous
 * sectors util an entry with a given type
 * is found or end of cluster is reached
 *
 * e.g:
 * parent_dir      -> /dir1 -> sector 149 
 * child_record(0) -> .     -> sector 149 
 * child_record(1) -> ..    -> sector 149
 * child_record(2) -> dir2  -> sector 149
 * child_record(3) -> dir3  -> sector 149
 * child_record(4) -> this  -> sector 150
 *
 * param cluster - logical cluster number
 * param type    - record type
 *
 * returns - contiguous record position in a cluster.
**/
DWORD lookup_cont_record_by_type(DWORD cluster, BYTE type) {
    // convert cluster to sector
    DWORD sector = cluster_to_log_sector(cluster);

    // calculate number of records that fits in sector
    int nr_of_records = records_per_sector();

    // max number of sectors from current sector to reach
    // clusters end boundary
    // since a cluster is usually formated as four sectors we
    // add SectorsPerCluster variable to our current sector variable
    // to calculate its cluster boundary
    int cluster_boundary = sector + superblock.SectorsPerCluster;

    // while in boundary
    while(sector <= cluster_boundary) {
        // record counter
        int i = 0;

        // read our cluster sectors
        read_sector(sector, buffer);

        // loop through records of current sector
        while(i < nr_of_records) {
            Record desc;

            // read our i(th) record from current sector
            memcpy(&desc, buffer + (RECORD_SIZE * i), RECORD_SIZE);

            // if record name equals to parameter name then we found our record
            // also make sure this record is valid checking its typeval 
            // typeval_invalido means that this record is empty
            if (desc.TypeVal == type) {
                
                // find out which sector we are counting from a zero-based index
                int curr_sector = (sector - cluster_boundary) + superblock.SectorsPerCluster;

                // calculate continuous logical position of this record accumulating
                // positions from previous sectors util a entry with a given condition
                // is reached
                // 
                // e.g:
                //
                // curr_dir ->
                // record(0) -> .    -> sector 1
                // record(1) -> ..   -> sector 1
                // record(2) -> dir2 -> sector 1
                // record(3) -> dir3 -> sector 1
                // record(4) -> this -> sector 2
                return curr_sector * nr_of_records + i;
            }

            i++;
        }

        sector++;
    }

    // unable to find record in cluster
    return FALSE;
}

/**
 * Lookup parent record descriptor by parent path.
 *
 * eg.: name -> /dir1/file1.txt
 *
 * parent -> /dir
 * name   -> file1.txt
 *
 * param name    - parent path that this function will try to match (/dir in example above)
 * param record  - if matched then this variable will store record found during lookup
 *
 * returns - TRUE if found FALSE otherwise.
**/
int lookup_parent_descriptor_by_name(char *name, Record *record) {
    // calculate name length just one time to 
    // reuse it later
    int name_len = strlen(name) + 1;

    // create an auxiliary array from name
    // acting as a buffer to strtok without
    // modifying external name variable passed
    // as reference to this function
    char aux_name[name_len];

    strncpy(aux_name, name, name_len);

    // if name equals to slash then it exists since 
    // its on root path
    if (strcmp(aux_name, "/") == 0) {
        return TRUE;

    // otherwise we must traverse all data sectors checking if path exists
    } else {
        // record type that will be reused while traversing
        Record desc;

        // calculate data cluster number from current directory
        DWORD cluster = curr_data_cluster();

        // tokenize name splitting by "/"
        char *token = strtok(aux_name, "/");

        // flag storing if name exists on disk 
        int exists = TRUE;

        do {
            if (token != NULL) {
                // look for name in first cluster storing descriptor
                // in record pointer
                exists = exists && lookup_descriptor_by_name(cluster, token, &desc);

                // if record could was found in cluster then select
                // next cluster in chain from firstCluster variable
                // and mark exists flag as true
                if (exists) cluster = desc.firstCluster;

                // otherwise it was not found and we
                // should mark exists flag as false
                else exists = FALSE;
            }

            // next token
            token  = strtok(NULL, "/");

        // while token is not null or exists flag is not false
        // keep looping
        } while(token != NULL && exists);

        // if found store descriptor in record parameter
        if (exists == TRUE) {
            memcpy(record, &desc, RECORD_SIZE);
        }

        // return lookup result
        return exists;
    }
}

/**
 * Check wheter a given name exists.
 *
 * returns - physical cluster size.
 * on error - returns -1 if name does not exists or 0 if it exists.
**/
int does_name_exists(char *name) {
    // calculate name length just one time to 
    // reuse it later
    int name_len = strlen(name) + 1;

    // create an auxiliary array from name
    // acting as a buffer to strtok without
    // modifying external name variable passed
    // as reference to this function
    char aux_name[name_len];

    strncpy(aux_name, name, name_len);

    // if name equals to slash then it exists since 
    // its on root path
    if (strcmp(aux_name, "/") == 0) {
        return TRUE;

    // otherwise we must traverse all data sectors checking if path exists
    } else {
        // record type that will be reused while traversing
        Record record;

        // if parent exists then this function call will
        // return TRUE otherwise false
        return lookup_parent_descriptor_by_name(name, &record);
    }
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
    int is_relative =  (starts_with_slash && is_eq_ignore_slash) || is_equal;

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
    strncpy(result->tail, tail, strlen(tail) + 1);
    strncpy(result->head, head, strlen(head) + 1);
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
DWORD fat_log_to_phys(DWORD lsector) {
    return (lsector -1) * SECTOR_SIZE;
}

/**
 * Convert phyisical FAT sector entry to logical sector entry.
 * 
 * returns  - logical sector entry in FAT.
**/
DWORD fat_phys_to_log(DWORD psector) {
    return superblock.pFATSectorStart + (DWORD)((double) psector / SECTOR_SIZE);
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

/**
 * Receives a cluster and a result buffer
 *
 * Saves the content of the cluster on the result buffer
**/
void read_cluster(int cluster, unsigned char *result) {
    int starting_sector = superblock.DataSectorStart + cluster * superblock.SectorsPerCluster;
    int sector_index;
    for(sector_index = 0; sector_index < superblock.SectorsPerCluster; sector_index++) {
        read_sector(starting_sector + sector_index, &result[sector_index * SECTOR_SIZE]);
    }
}

/**
 * Receives a cluster and a content buffer
 *
 * Saves the content of the buffer on the cluster
**/
void write_on_cluster(int cluster, unsigned char *content) {
    int starting_sector = superblock.DataSectorStart + cluster * superblock.SectorsPerCluster;
    int sector_index;
    for(sector_index = 0; sector_index < superblock.SectorsPerCluster; sector_index++) {
        write_sector(starting_sector + sector_index, &content[sector_index * SECTOR_SIZE]);
    }
}

/**
 * Check if the max number of files was reached
 * If the max num is reached, you can not open a new file
 *
 * Returns 0 if can open; -1 if can't
**/
int can_open() {
    return (num_opened_files < MAX_OPENED_FILES) ? SUCCESS : ERROR;
}

/**
 * Save a record on the list of opened files
 *
 * Returns -1 on Error; index of the opened file on Success
**/
int save_as_opened(Record* record) {
    if (can_open() == ERROR) {
        return ERROR;
    }

    // save the record of file in the first free position of opened_files
    int i;
    for (i = 0; i < MAX_OPENED_FILES; i++) {
        if (opened_files[i].is_used == FALSE) {
            // copy to the free position found
            memcpy(&opened_files[i].file, &record, sizeof(Record));
            // set the position as used
            opened_files[i].is_used = TRUE;
            // increase the opened files counter
            num_opened_files++;
            return i;
        }
    }
    
    return ERROR;
    
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
