#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "../include/apidisk.h"
#include "../include/t2fs.h"
#include "../include/fs_helper.h"

/*-----------------------------------------------------------------------------
 * function definition
-----------------------------------------------------------------------------*/

int identify2 (char *name, int size) {
    return SUCCESS;
}

FILE2 create2 (char *filename) {
    // stores if read and write was successfull
    int can_read_write = SUCCESS;
    //print_disk();
    
    // extract path head and tail
    Path *path = malloc(sizeof(Path));
    path_from_name(filename, path);

    // return error if parent path does not exists
    if (!does_name_exists(path->tail))
        return ERROR;

    // get the descriptor for parent folder
    Record parent_dir;
    lookup_parent_descriptor_by_name(path->tail, &parent_dir);

    // find first free fat physical sector entry
    int p_free_sector = phys_fat_first_fit();
    //printf("\nFREE SECTOR ON CREATE2 = %i", p_free_sector);

    // create the record for the new file
    Record file;
    file.TypeVal = TYPEVAL_REGULAR;
    file.bytesFileSize = 0;
    file.clustersFileSize = 1;
    strncpy(file.name, path->head, FILE_NAME_SIZE);
    file.firstCluster = p_free_sector;

    // Here we insert the entry of the file on the parent directory
    
    // buffer to read the content of parent dir cluster
    unsigned char content[SECTOR_SIZE * superblock.SectorsPerCluster];
    read_cluster(parent_dir.firstCluster, content);

    int i;

    // flag to check if a space to write was found
    int able_to_write = FALSE;

    Record tmp_record;
    for (i = 0; i < records_per_sector() * superblock.SectorsPerCluster; i++) {
        int position_on_cluster = i * RECORD_SIZE;

        // Copy the current record do parent dir to tmp_record
        memcpy(&tmp_record, &content[position_on_cluster], RECORD_SIZE);

        // Just need to check for duplications if it isn't invalid
        if (tmp_record.TypeVal != TYPEVAL_INVALIDO) {
            // Return error if the file already exists
            if (strcmp(tmp_record.name, file.name) == 0) {
                return ERROR;
            }

        // If it's invalid = it's free (ie, we can write on it)
        } else {
            // copy the content of file to the actual position on cluster
            memcpy(&content[position_on_cluster], &file, RECORD_SIZE);
            // write the modified cluster (with the new file) 
            write_on_cluster(parent_dir.firstCluster, content);
            // doesn't need to iterate anymore
            able_to_write = TRUE;
            
            break;
        }
    }

    // Directory is full
    if (able_to_write == FALSE)
        return ERROR;

    // convert physical fat sector entry to logical fat sector entry
    int l_fat_free_sector = fat_phys_to_log(p_free_sector);

    // read from logical sector and fill up buffer
    can_read_write = read_sector(l_fat_free_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;

    // calculate cluster size from free physical sector entry
    BYTE p_free_cluster = p_free_sector * superblock.SectorsPerCluster;

    // fill buffer area with END_OF_FILE marking last sector
    // as END_OF_FILE (value 0xFFFFFFFF) since directories occupy 
    // one cluster by specs
    DWORD eof = END_OF_FILE;
    memcpy(buffer + p_free_cluster, &eof, FAT_ENTRY_SIZE);

    // write this sector back to disk in FAT marking current
    // fat entry as END_OF_FILE
    can_read_write = write_sector(l_fat_free_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;

    //print_disk();
    
	// if possible, save as opened
	// otherwise, returns a error
	return save_as_opened(file);
}

int delete2 (char *filename) {
    // stores if read and write was successfull
    int can_read_write = SUCCESS;

	// extract path head and tail
    Path *path = malloc(sizeof(Path));
    path_from_name(filename, path);

    // return error if parent path does not exists
    if (!does_name_exists(path->tail))
        // path does not exists
        return ERROR;

    // get the descriptor for parent folder
    Record parent_dir;
    lookup_parent_descriptor_by_name(path->tail, &parent_dir);

    // find the to-be-deleted file inside of parent dir
	Record file;
	if (!lookup_descriptor_by_name(parent_dir.firstCluster, path->head, &file))
		// file does not exists
		return ERROR;  

	if (!(file.TypeVal == TYPEVAL_REGULAR || file.TypeVal == TYPEVAL_LINK))
		// Is not a regular file or softlink
		return ERROR;

	// Here we delete the entry of the file on the parent directory
    
    // buffer to read the content of parent dir cluster
    unsigned char content[SECTOR_SIZE * superblock.SectorsPerCluster];
    read_cluster(parent_dir.firstCluster, content);

    int i;

    // flag to check if a space to write was found
    int found = FALSE;

    Record tmp_record;

    for (i = 0; i < records_per_sector() * superblock.SectorsPerCluster; i++) {
        int position_on_cluster = i * RECORD_SIZE;
        // Copy the current record do parent dir to tmp_record
        memcpy(&tmp_record, &content[position_on_cluster], RECORD_SIZE);
        if (strcmp(tmp_record.name, file.name) == 0) {
            // sets file type to invalid (ie, it is now a free entry)
            file.TypeVal = TYPEVAL_INVALIDO;

            memcpy(&content[position_on_cluster], &file, RECORD_SIZE);
            // write the modified cluster (without the file) 
            write_on_cluster(parent_dir.firstCluster, content);
            found = TRUE;
        }
    }

    if (found == FALSE)
    	return ERROR;

        // convert physical fat sector entry to logical fat sector entry
    int l_fat_file_sector = fat_phys_to_log(file.firstCluster);
    //printf("\nFILE SECTOR ON DELETE2 = %i", file.firstCluster);

    // read from logical sector and fill up buffer
    can_read_write = read_sector(l_fat_file_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;

    // calculate cluster size from free physical sector entry
    BYTE p_file_cluster = file.firstCluster * superblock.SectorsPerCluster;

    // fill buffer area with FREE_CLUSTER
    DWORD eof = FREE_CLUSTER;
    memcpy(buffer + p_file_cluster, &eof, FAT_ENTRY_SIZE);

    // write this sector back to disk in FAT marking current
    // fat entry as END_OF_FILE
    can_read_write = write_sector(l_fat_file_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;

    return SUCCESS;
}

FILE2 open2 (char *filename) {
	// extract path head and tail
    Path *path = malloc(sizeof(Path));
    path_from_name(filename, path);

    // return error if parent path does not exists
    if (!does_name_exists(path->tail))
        // path does not exists
        return ERROR;

    // get the descriptor for parent folder
    Record parent_dir;
    lookup_parent_descriptor_by_name(path->tail, &parent_dir);

    // find the to-be-deleted file inside of parent dir
	Record file;
	if (!lookup_descriptor_by_name(parent_dir.firstCluster, path->head, &file))
		// file does not exists
		return ERROR;  

	if (!(file.TypeVal == TYPEVAL_REGULAR || file.TypeVal == TYPEVAL_LINK))
		// Is not a regular file or softlink
		return ERROR;

	// if possible, save as opened
	// otherwise, returns a error
	return save_as_opened(file);
}

int close2 (FILE2 handle) {
	// Check if handle is inside of boundaries
	if (handle < 0)
		return ERROR;
	if (handle >= MAX_OPENED_FILES)
		return ERROR;
	// Check if the passed handle has a file 
	if (opened_files[handle].is_used == FALSE)
		return ERROR;

	//opened_files[handle].file = (Record) NULL;
	opened_files[handle].is_used = FALSE;

	num_opened_files--;
    return SUCCESS;
}

int read2 (FILE2 handle, char *buffer, int size) {
	// get the file from the opened list
	Record file = opened_files[handle].file; 
	int current_pointer = opened_files[handle].current_pointer;
	
	// creates a buffer to read the content
	unsigned char content[file.clustersFileSize * SECTOR_SIZE * superblock.SectorsPerCluster];
	
	// read cluster by cluster and saves on the buffer
	int cluster = file.firstCluster;
	int i;
	for (i = 0; i < file.clustersFileSize; i++) {
		read_cluster(cluster, &content[i * SECTOR_SIZE * superblock.SectorsPerCluster]);
		cluster = local_fat[cluster];
	}

	// size = min(size, difference_lenght)
	// where difference_length is the size of bytes from the current_pointer
	//		to the end of the file
	int difference_length = file.bytesFileSize - current_pointer;
	size = difference_length < size ? difference_length : size;

	// copy the read content to the buffer
	memcpy(buffer, &content[current_pointer], size);

	// increases the current pointer
	opened_files[handle].current_pointer += size;

    return size;
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

/**
 * Creates a new directory. 
 *
 * param pathname - absolute or relative path for new directory
 * 
 * returns - SUCCESS if create FALSE otherwise.
**/
int mkdir2 (char *pathname) {
    // stores if read and write was successfull
    int can_read_write = SUCCESS;

    // extract path head and tail
    Path *path = malloc(sizeof(Path));
    path_from_name(pathname, path);

    // make sure parent path exists and
    // current name doesnt exists 
    int valid = does_name_exists(path->tail) && !does_name_exists(path->head);

    // unable to locate parent path in disk or
    // current name exists in disk
    // then return an error
    if (!valid) {
        printf("error - path doesnt exist or file name already exists\n");
        return ERROR;
    }

    // find first free fat physical sector entry
    int p_free_sector = phys_fat_first_fit();
    
    // convert physical fat sector entry to logical fat sector entry
    int l_fat_free_sector = fat_phys_to_log(p_free_sector);

    // read from logical sector and fill up buffer
    can_read_write = read_sector(l_fat_free_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;

    // calculate cluster size from free physical sector entry
    BYTE p_free_cluster = p_free_sector * superblock.SectorsPerCluster;

    // fill buffer area with END_OF_FILE marking last sector
    // as END_OF_FILE (value 0xFFFFFFFF) since directories occupy 
    // one cluster by specs
    DWORD eof = END_OF_FILE;
    memcpy(buffer + p_free_cluster, &eof, FAT_ENTRY_SIZE);

    // write this sector back to disk in FAT marking current
    // fat entry as END_OF_FILE
    can_read_write = write_sector(l_fat_free_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;

    // create current directory (head from path_from_name func)
    Record dir;
    dir.TypeVal = TYPEVAL_DIRETORIO;
    dir.bytesFileSize = phys_cluster_size();
    dir.clustersFileSize = 1;
    strncpy(dir.name, path->head, FILE_NAME_SIZE);
    dir.firstCluster = p_free_sector;

    // find parent directory by tail
    Record parent_dir;
    lookup_parent_descriptor_by_name(path->tail, &parent_dir);

    // convert parent_dir cluster to logical sector
    DWORD l_parent_sector = cluster_to_log_sector(parent_dir.firstCluster);

    // find free entry within parent cluster
    DWORD free_entry = lookup_cont_record_by_type(parent_dir.firstCluster, TYPEVAL_INVALIDO);

    // calculate number of records per sector
    int nr_of_records = records_per_sector();

    // calculate free entry logical sector
    DWORD l_free_entry_sector = free_entry / nr_of_records;

    // maps logical entry sector
    DWORD l_parent_free_entry_sector = l_free_entry_sector + l_parent_sector;

    // read from logical entry sector
    can_read_write = read_sector(l_parent_free_entry_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;

    // maps logical free entry to physical free entry
    DWORD p_free_entry = (free_entry - l_free_entry_sector * superblock.SectorsPerCluster) * RECORD_SIZE;

    // fill buffer phyisical entry position with directory content
    memcpy(buffer + p_free_entry, &dir, RECORD_SIZE);

    // write parent sector back to disk
    can_read_write = write_sector(l_parent_free_entry_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;

    // convert free physical fat sector to logical sector logical data sector
    DWORD l_data_free_sector = cluster_to_log_sector(p_free_sector);

    // read logical free data sector to cleanup buffer
    read_sector(l_data_free_sector, buffer);
    
    // create self pointer '.'
    Record self;
    self.TypeVal = TYPEVAL_DIRETORIO;
    strcpy(self.name, ".");
    self.bytesFileSize = phys_cluster_size();
    self.clustersFileSize = 1;
    self.firstCluster = p_free_sector;

    // create parent pointer '..'
    Record parent;
    parent.TypeVal = TYPEVAL_DIRETORIO;
    strcpy(parent.name, "..");
    parent.bytesFileSize = phys_cluster_size();
    parent.clustersFileSize = 1;
    parent.firstCluster = parent_dir.firstCluster;

    // create add self pointer to buffer
    memcpy(buffer, &self, RECORD_SIZE);
    
    // create add parent pointer to buffer
    memcpy(buffer + RECORD_SIZE, &parent, RECORD_SIZE);

    // write buffer within logical data sector
    write_sector(l_data_free_sector, buffer);

    // release resources for path
    free(path);

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
    return (0);
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
