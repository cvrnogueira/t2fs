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
        int position_on_cluster = i * sizeof(Record);
        // Copy the current record do parent dir to tmp_record
        memcpy(&tmp_record, &content[position_on_cluster], sizeof(Record));
        // Just need to check for duplications if it isn't invalid
        if (tmp_record.TypeVal != TYPEVAL_INVALIDO) {
            // Return error if the file already exists
            if (strcmp(tmp_record.name, file.name) == 0) {
                return ERROR;
            }
        // If it's invalid = it's free (ie, we can write on it)
        } else {
            // copy the content of file to the actual position on cluster
            memcpy(&content[position_on_cluster], &file, sizeof(Record));
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

    // convert phyisical sector entry to logical sector entry
    int l_free_sector = fat_phys_to_log(p_free_sector);

    // read from logical sector and fill up buffer
    read_sector(l_free_sector, buffer);

    // calculate cluster size from free phyisical sector entry and
    // set buffer position to previous calculated cluster
    BYTE free_cluster = ((int) buffer) + p_free_sector * superblock.SectorsPerCluster;

    // fill buffer area with END_OF_FILE marking last sector
    // as END_OF_FILE (value 0xFFFFFFFF)
    DWORD eof = END_OF_FILE;
    memcpy(&free_cluster, &eof, FAT_ENTRY_SIZE);

    // write this sector back to disk in FAT
    write_sector(l_free_sector, buffer);
    
    //print_disk();
    return (SUCCESS);
}

int delete2 (char *filename) {
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
        int position_on_cluster = i * sizeof(Record);
        // Copy the current record do parent dir to tmp_record
        memcpy(&tmp_record, &content[position_on_cluster], sizeof(Record));
        if (strcmp(tmp_record.name, file.name) == 0) {
            // sets file type to invalid (ie, it is now a free entry)
            file.TypeVal = TYPEVAL_INVALIDO;

            memcpy(&content[position_on_cluster], &file, sizeof(Record));
            // write the modified cluster (without the file) 
            write_on_cluster(parent_dir.firstCluster, content);
            found = TRUE;
        }
    }

    if (found == FALSE)
    	return ERROR;

    // get file physical sector entry
    int p_file_sector = file.firstCluster;
    //printf("\nFILE SECTOR = %i", p_file_sector);

    // convert phyisical sector entry to logical sector entry
    int l_file_sector = fat_phys_to_log(p_file_sector);

    // read from logical sector and fill up buffer
    read_sector(l_file_sector, buffer);

    // calculate cluster size from free phyisical sector entry and
    // set buffer position to previous calculated cluster
    BYTE file_cluster = ((int) buffer) + p_file_sector * superblock.SectorsPerCluster;

    // fill buffer area with FREE_CLUSTER 
    DWORD eof = FREE_CLUSTER;
    memcpy(&file_cluster, &eof, FAT_ENTRY_SIZE);

    // write this sector back to disk in FAT
    write_sector(l_file_sector, buffer);

    return SUCCESS;
}

FILE2 open2 (char *filename) {
    return (0);
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

int unusedEntryDir(DWORD cluster){
    DWORD entry = 0;
    char candidate[MAX_FILE_NAME_SIZE];
    int numOfSectors = 0;
    while(numOfSectors < superblock.SectorsPerCluster){
        if(read_sector((cluster*superblock.SectorsPerCluster + superblock.DataSectorStart + numOfSectors), buffer) != 0){
            return -1;
        }
        while(entry <= SECTOR_SIZE - sizeof(Record)){
            strncpy(candidate,(const char*)(buffer + entry + 1), MAX_FILE_NAME_SIZE);
            entry = entry + sizeof(Record);
            if(strcmp(candidate,"") == 0){
                if((*(BYTE *)(buffer + entry - sizeof(Record))) ==  TYPEVAL_INVALIDO){
                    return (numOfSectors*SECTOR_SIZE + entry - sizeof(Record))/sizeof(Record);
                }
            }
        }
        numOfSectors++;
        entry = 0;
    }
    return -1;
}

int mkdir2 (char *pathname) {
    // extract path head and tail
    Path *path = malloc(sizeof(Path));
    path_from_name(pathname, path);

    // make sure parent path exists and
    // current name doesnt exists 
    int exists = does_name_exists(path->tail) && !does_name_exists(path->head);

    // unable to locate parent path in disk or
    // current name exists in disk
    // then return an error
    if (!exists) return ERROR;

    // find parent directory by tail
    Record parent_dir;
    lookup_parent_descriptor_by_name(path->tail, &parent_dir);

    // find free entry within parent cluster
    DWORD free_entry = lookup_cont_record_by_type(parent_dir.firstCluster, TYPEVAL_INVALIDO);

    // find first free fat physical sector entry
    int p_free_sector = phys_fat_first_fit();

    // convert phyisical sector entry to logical sector entry
    int l_free_sector = fat_phys_to_log(p_free_sector);

    // read from logical sector and fill up buffer
    read_sector(l_free_sector, buffer);

    // calculate cluster size from free phyisical sector entry and
    // set buffer position to previous calculated cluster
    BYTE free_cluster = buffer + p_free_sector * superblock.SectorsPerCluster;

    // fill buffer area with END_OF_FILE marking last sector
    // as END_OF_FILE (value 0xFFFFFFFF) since directories occupy 
    // one cluster by specs
    DWORD eof = END_OF_FILE;
    memcpy(&free_cluster, &eof, FAT_ENTRY_SIZE);

    // write this sector back to disk in FAT
    write_sector(l_free_sector, buffer);

    // create current directory (head from path_from_name func)
    Record dir;
    dir.TypeVal = TYPEVAL_DIRETORIO;
    dir.bytesFileSize = phys_cluster_size();
    strncpy(dir.name, path->head, FILE_NAME_SIZE);
    dir.firstCluster = p_free_sector;

    // TODO: 
    // - write directory to data sector
    // - create . and .. directories and write to data sector
    // - update parent directory
    // - missing '/' (root) checks

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
