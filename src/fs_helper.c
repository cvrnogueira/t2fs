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
	num_opened_dirs = 0;

    set_local_fat();
}

/*
 *  Refresh in-memmory fat table.
 *
 * This function is declared here because is not supposed 
 * to be accessed from outside.
*/
int set_local_fat() {
    // allocate the necessary memory for a local instance of FAT
    local_fat = malloc(SECTOR_SIZE * superblock.DataSectorStart - superblock.pFATSectorStart);
    
    int index;

    int can_read_write = SUCCESS;

    // temp variable to save the content of a sector
    unsigned char sector_content[SECTOR_SIZE];

    for (index = superblock.pFATSectorStart; index < superblock.DataSectorStart; index++) {
        int fat_index = (index - superblock.pFATSectorStart) * SECTOR_SIZE/4;
        
        can_read_write = read_sector(index, sector_content);
        
        if (can_read_write != SUCCESS) return ERROR;

        memcpy(&local_fat[fat_index], sector_content, SECTOR_SIZE);
    }

    return SUCCESS;
}

/**
 * Save a dword on a given position of local FAT
 * Also update the FAT position on disk according to the local FAT
 *
 * Returns the result of write_sector (to raise an error, if necessary)
**/
int set_value_to_fat(int position, DWORD value) {
    int index;
    // calculates the number of entries per sector on FAT
    int entries_per_sector = SECTOR_SIZE / FAT_ENTRY_SIZE;

	if (local_fat[position] == 0xFFFFFFFE)//we have to check if the current cluster isn't a bad one
		return ERROR;

    // save the value on local fat
    local_fat[position] = value;    



    // loop on FAT
    for(index = superblock.pFATSectorStart; index < superblock.DataSectorStart; index++) {
        // a entry has 4 bytes; a sector has 256 bytes
        // so we have 256/4 = 64 entries per sector
        // we can only write a sector each time, so we need to get
        //     64 entries each time (a full sector) and write it
        // therefore sector_index goes 0, 64, 128, 192, etc
        int sector_index = (index - superblock.pFATSectorStart) * entries_per_sector;
        if (write_sector(index, (unsigned char*)  &local_fat[sector_index]) == ERROR)
            return ERROR;
    }

    return SUCCESS;
}

/**
 * Initialize curr_dir position to data sector after root sectors.
**/
int initialize_curr_dir(Superblock *superblock) {
    curr_dir = superblock->DataSectorStart + superblock->RootDirCluster * superblock->SectorsPerCluster;
    return SUCCESS;
}


/**
Initialize opened dir/files vector structs
**/
void initialize_opened_structs()
{

	int i;
	for (i = 0; i < MAX_OPENED_FILES; i++)
		opened_files[i].is_used = FALSE;
	for (i = 0; i < MAX_OPENED_DIRS; i++)
		opened_dirs[i].is_used = FALSE;
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
        if (read_sector(sector, buffer) != SUCCESS) return ERROR;

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
        if (read_sector(sector, buffer) != SUCCESS) return ERROR;

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
 * Last character in string.
 *
 * returns - last character in string or empty character (ie: (char) 0 ) if not possible
**/
char last_char(char *value) {
    if (value == NULL) return '\0';

    int value_len = strlen(value);

    int idx = value_len - 1;

    return idx >= 0 ? value[idx] : (char) 0;
}

/**
 * Prepend str2 in str1.
**/
void str_prepend(char *str1, char *str2) {
    char *tmp = strdup(str1);

    strncpy(str1, str2, strlen(str2) + 1);

    strncat(str1, tmp, strlen(tmp));
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
int path_from_name(char *name, Path *result) {
    // if name is only slash then we dont need to worry
    // about extracting path we can just set curr_dir to
    // root dir cluster
    if (strcmp(name, "/") == 0) {
        strncpy(result->tail, "/", 1);

        return SUCCESS;
    }

    // calculate name length
    const int name_len = strlen(name);

    // validate name length
    if (name_len <= 0 || name_len >= MAX_PATH_SIZE) {
        return ERROR;
    }

    // allocate head, tail character
    // arrays that will fill path struct later on
    char head[name_len]; 
    char tail[name_len];
    char both[name_len];

    // empty string arrays
    head[0] = 0;
    tail[0] = 0;
    both[0] = 0;

    // extract first and second character from name parameter
    char fst_char_raw = name_len > 0 ? name[0] : (char) 0;
    char snd_char_raw = name_len > 1 ? name[1] : (char) 0;

    // is only .. without trailling slash
    int parent_only = fst_char_raw == '.' && snd_char_raw == '.' && name_len == 2;

    // check if name start with ../
    int starts_with_dot2 = fst_char_raw == '.' && snd_char_raw == '.';

    // check if name starts with ./
    int starts_with_dot_slash = fst_char_raw == '.' && snd_char_raw == '/';

    // remove .. or ./ from path if name parameter starts with it
    char *sanitized_name = NULL;
    if (starts_with_dot_slash) sanitized_name = name + 2;
    else if (parent_only) sanitized_name = name + 2;
    else if (starts_with_dot2) sanitized_name = name + 3;
    else sanitized_name = name;

    //  check if name starts with slash
    int starts_with_slash = fst_char_raw == '/';

    // after removing ./ or .. from name include it on tail so
    // it can be used by tokenizer loop later
    if (starts_with_dot2 || parent_only) strncat(tail, "../", 3);
    else if (starts_with_dot_slash || !starts_with_slash) strncat(tail, "./", 2);

    // create an auxiliary array from name
    // acting as a buffer to strtok without
    // modifying external name variable passed
    // as reference to this function
    char aux_token[strlen(sanitized_name)];
    strcpy(aux_token, sanitized_name);

    // tokenize name splitting by "/"
    char *token = strtok(aux_token, "/");

    // iterate through remaining path using tokenizer to 
    // concating current head to tail until no 
    // slash character is found
    //
    // eg.:
    // 
    // name -> /home/aluno/sisop/t2fs 
    // ite(1): tail -> /home | head -> aluno
    // ite(2): tail -> /home/aluno | head -> sisop
    // ite(3): tail -> /home/aluno/sisop | head -> t2fs
    do {
        
        if (token != NULL && strlen(head) >= 1) {
            strncat(tail, head, strlen(head) + 1);
        }

        if (token != NULL) {
            strncpy(head, token, strlen(token) + 1);
        }

        token = strtok(NULL, "/");

        char tail_last_char = last_char(tail);

        if (token != NULL && tail_last_char != '/') {
            strncat(tail, "/", 1);
        }

    } while (token != NULL);

    // fill result with generated data
    // from tokenizer steps
    strncpy(result->tail, tail, strlen(tail) + 1);
    strncpy(result->head, head, strlen(head) + 1);

    // concat tail
    strncat(both, tail, strlen(tail) + 1);

    // calculate head length
    int head_len = strlen(head);

    // if path is relative we should not concat slashes at end=

    // concat head if not empty and if tail is not slash-ending
    if (head_len > 0) {
        char tail_last_char = last_char(tail);

        if (tail_last_char != '/') strncat(both, "/", 1);

        strncat(both, head, strlen(head) + 1);
    }

    // copy concatenation to both attribute in result structure
    strncpy(result->both, both, strlen(both) + 1);

    return SUCCESS;
}


/**
 * Lookup record descriptor by its cluster number.
 *
 * param cluster - logical cluster number
 * param record  - if matched then this variable will store record found during lookup
 *
 * returns - TRUE if found FALSE otherwise.
**/
int lookup_descriptor_by_cluster(DWORD cluster, Record *record) {
    // find parent directory by .. logical reference
    Record parent_dir;
    lookup_descriptor_by_name(cluster, "..", &parent_dir);

    // allocate a buffer for storing cluster content
    unsigned char content[SECTOR_SIZE * superblock.SectorsPerCluster];
    read_cluster(parent_dir.firstCluster, content);

    // loop thourgh parent directory to our entry
    int i;
    Record tmp_record;
    for (i = 0; i < records_per_sector() * superblock.SectorsPerCluster; i++) {

        // calculate cluster position
        int position_on_cluster = i * RECORD_SIZE;

        // Copy the current record do parent dir to tmp_record
        memcpy(&tmp_record, &content[position_on_cluster], RECORD_SIZE);

        // check if current entry is equals to cluster then we 
        // have our record
        if (tmp_record.firstCluster == cluster) {
            memcpy(record, &tmp_record, RECORD_SIZE);

            return SUCCESS;
        }

    }

    // unable to find record then return error
    return ERROR;
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
 * on error - returns ERROR if cant read fat partition or theres no free entry.
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
            const DWORD entry_pos = ((int) buffer) + entry;

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
 * Read logical cluster and saves its content to the result buffer
 *
 * on error - returns ERROR if cant read from cluster otherwise SUCCESS.
**/
int read_cluster(int cluster, unsigned char *result) {
    int starting_sector = superblock.DataSectorStart + cluster * superblock.SectorsPerCluster;
    
    int sector_index;
    
    int can_read_write = SUCCESS;

	if (local_fat[cluster]== 0xFFFFFFFE)//testing if it is a bad block
		return ERROR;

    for(sector_index = 0; sector_index < superblock.SectorsPerCluster; sector_index++) {
        can_read_write = read_sector(starting_sector + sector_index, &result[sector_index * SECTOR_SIZE]);

        if (can_read_write != SUCCESS) return ERROR;
    }

    return SUCCESS;
}

/**
 * Writes content to logical cluster
 *
 * on error - returns ERROR if cant write to cluster otherwise SUCCESS.
**/
int write_cluster(int cluster, unsigned char *content) {
    int starting_sector = superblock.DataSectorStart + cluster * superblock.SectorsPerCluster;
    
    int sector_index;

    int can_read_write = SUCCESS;

    for(sector_index = 0; sector_index < superblock.SectorsPerCluster; sector_index++) {
        can_read_write = write_sector(starting_sector + sector_index, &content[sector_index * SECTOR_SIZE]);

        if (can_read_write != SUCCESS) return ERROR;
    }

    return SUCCESS;
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
int save_as_opened(Record record, char* path) {
    if (can_open() == ERROR) {
        return ERROR;
    }


    // save the record of file in the first free position of opened_files
    int i;
    for (i = 0; i < MAX_OPENED_FILES; i++) {
        if (opened_files[i].is_used == FALSE) {
            // copy to the free position found
            memcpy(&(opened_files[i].file), &record, sizeof(Record));
            
            // set the position as used
            opened_files[i].is_used = TRUE;
            
            // set current pointer on the start of the file
            opened_files[i].current_pointer = 0;
            
            // set path to the record
			strcpy(opened_files[i].path, path);
            
            // increase the opened files counter
            num_opened_files++;
            return i;
        }
    }
    
    return ERROR;
}


int save_as_opened_dir(Record record, char* pathname)
{

	int i;
	if (num_opened_dirs >= MAX_OPENED_DIRS)
		return ERROR;
	else
	{

		int address = findValidEntry(record, 0);
		if (address == -1)
			return ERROR;


		if (unusedDirHandles) //if a handle has been freed, we have to search the array for it 
		{
			for (i = 0; i < MAX_OPENED_DIRS; i++)
				if (opened_dirs[i].is_used == FALSE)
				{
					opened_dirs[i].record = record;
					opened_dirs[i].is_used = TRUE;
					strcpy(opened_dirs[i].path, pathname);
					opened_dirs[i].current_pointer = address;
					num_opened_dirs++;
					unusedDirHandles--;
					return i;
				}
		}
		else //else, it's the trivial case
		{
			i = num_opened_dirs;
			opened_dirs[i].record = record;
			opened_dirs[i].is_used = TRUE;
			opened_dirs[i].current_pointer = address;
			opened_dirs[i].path = malloc(MAX_PATH_SIZE * sizeof(char));
			strcpy(opened_dirs[i].path, pathname);
			num_opened_dirs++;
			return i;
		}
	}

	return ERROR;

}

int findValidEntry(Record record, int end)
{
	int cluster_size = SECTOR_SIZE * superblock.SectorsPerCluster;
	unsigned char result[cluster_size];
	struct t2fs_record descriptor;
	int address = end;
	if (address >= cluster_size)
		return ERROR;
	if (address == ERROR)
		return ERROR;
	if (read_cluster(record.firstCluster, result) != SUCCESS) return ERROR;
	// 64 = tamanho da estrutura t2fs_record
	do
	{
		memcpy(&descriptor, &result[address], sizeof(descriptor));
		address += 64;
	} while (descriptor.TypeVal == TYPEVAL_INVALIDO && address <= cluster_size);

	if (address >= cluster_size)
	{
		address = 0;
		do
		{
			memcpy(&descriptor, &result[address], sizeof(descriptor));
			address += 64;
		} while (descriptor.TypeVal == TYPEVAL_INVALIDO && address <= cluster_size);
		if (address >= cluster_size)
			return ERROR;
		else
			return address;
	}
	else
		return address;

}

Record* findLinkRecord(Record link)
{

	Path* path;
	path = malloc(sizeof(Path));
	path = findLinkPath(link);
	

	if (path == NULL)
		return NULL;

	Record parent_dir;
	lookup_parent_descriptor_by_name(path->tail, &parent_dir);

	// find the record
	Record* file = malloc(sizeof(Record));
	if (!lookup_descriptor_by_name(parent_dir.firstCluster, path->head, file))
		{ 
		free(path);
		return NULL;
		}
	else
	{
		free(path);
		return file;
	}
}


Path* findLinkPath(Record link)
{
	unsigned char* name = malloc(sizeof(char) * phys_cluster_size());
	if (read_cluster(link.firstCluster, name) != SUCCESS) return NULL;
	Path* path = malloc(sizeof(Path));
	if (path_from_name((char*)name, path) != SUCCESS) {
		free(path);
		return NULL;
	}
	return path;
}



/**
 * Helper functions to print data, fat and super blocks from disk.
**/
void print_fat() {
    set_local_fat();
    int i;
    printf("\n");
    for (i = 0; i < 20; i++) {
        printf("%d: %02X\n", i, local_fat[i]);
    }
}

/**
 * Helper functions to print data, fat and super blocks from disk.
**/
void print_descriptor(struct t2fs_record descriptor, int tab) {
    int tabix;
    for (tabix = 0; tabix < tab; tabix++)
         printf("\t");
    printf("%s = %u bytes (%u clusters)  (%u custer index) \n", descriptor.name, descriptor.bytesFileSize, descriptor.clustersFileSize, descriptor.firstCluster);
}

/**
 * Helper functions to print data, fat and super blocks from disk.
**/
void print_dir(DWORD cluster, int tab) {
    int cluster_size = SECTOR_SIZE * superblock.SectorsPerCluster;
    
    BYTE result[cluster_size];
    
    struct t2fs_record descriptor;  
    
    int index; // Descriptor index

    read_cluster(cluster, result);

    // 64 = tamanho da estrutura t2fs_record
    for(index = 0; index < cluster_size / 64; index++) {
        memcpy(&descriptor, &result[index * 64], sizeof(descriptor));
	
        if (descriptor.TypeVal != TYPEVAL_INVALIDO) {
            print_descriptor(descriptor, tab);
        }
        
        if (descriptor.TypeVal == TYPEVAL_DIRETORIO && index > 1) {
            print_dir(descriptor.firstCluster, tab+1);
        }
    }
}

/**
 * Helper functions to print data, fat and super blocks from disk.
**/
void print_disk() {
    printf("\nDISK\n");
    print_dir(superblock.RootDirCluster, 0);
}

/**
 * Helper functions to print data, fat and super blocks from disk.
**/
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
