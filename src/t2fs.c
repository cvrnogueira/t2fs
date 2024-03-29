#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "../include/apidisk.h"
#include "../include/t2fs.h"
#include "../include/fs_helper.h"

/**
 * Creates a new archive.
 * 
 * returns - File handle if possible (positive number) ERROR otherwise. 
 **/
FILE2 create2 (char *filename) {
    // extract path head and tail
    Path *path = malloc(sizeof(Path));
    if (path_from_name(filename, path) != SUCCESS) {
        free(path);
        return ERROR;
    }

    // return error if parent path does not exist
    if (!does_name_exists(path->tail))
        return ERROR;

    // get the descriptor for parent folder
    Record parent_dir;
    lookup_parent_descriptor_by_name(path->tail, &parent_dir);

    // find first free fat physical sector entry
    int p_free_sector = phys_fat_first_fit();
    
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
    
    if (read_cluster(parent_dir.firstCluster, content) != SUCCESS) return ERROR;

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
           
			if (strcmp(tmp_record.name, file.name) == 0) { //if file already exists, delete the content which belongs to the original
				int clusterCounter;
				int cluster_to_delete = tmp_record.firstCluster;
				for (clusterCounter = 0; clusterCounter < tmp_record.clustersFileSize; clusterCounter++) {
					int tmp_cluster = local_fat[cluster_to_delete];
					if (clusterCounter == 0)
					{
						if (set_value_to_fat(cluster_to_delete, EOF) != SUCCESS)
							return ERROR;
					}
					else
					{
						if (set_value_to_fat(cluster_to_delete, FREE_CLUSTER) != SUCCESS)
							return ERROR;
					}
							
					cluster_to_delete = tmp_cluster;
				}
				file.bytesFileSize = 0;
				file.clustersFileSize = 1;

				// copy the content of file to the actual position on cluster
				memcpy(&content[position_on_cluster], &file, RECORD_SIZE);

				// write the modified cluster (with the new file) 
				write_cluster(parent_dir.firstCluster, content);

				// doesn't need to iterate anymore
				able_to_write = TRUE;

				break;
            }

        // If it's invalid = it's free (ie, we can write on it)
        } 
		else 
		{
            // copy the content of file to the actual position on cluster
            memcpy(&content[position_on_cluster], &file, RECORD_SIZE);
            
            // write the modified cluster (with the new file) 
            write_cluster(parent_dir.firstCluster, content);
            
            // doesn't need to iterate anymore
            able_to_write = TRUE;
            
            break;
        }
    }

    // parent directory is full
    if (able_to_write == FALSE)
        return ERROR;

    // sets EOF to the found free index of FAT
    if (set_value_to_fat(p_free_sector, EOF) != SUCCESS)
    	return ERROR;

    // release resources for path
    free(path);

	// if possible, save as opened
	// otherwise, returns a error
	return save_as_opened(file, filename);
}

int delete2 (char *filename) {
	// extract path head and tail
    Path *path = malloc(sizeof(Path));
    if (path_from_name(filename, path) != SUCCESS) {
        free(path);
        return ERROR;
    }

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
    if (read_cluster(parent_dir.firstCluster, content) != SUCCESS) return ERROR;

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
            write_cluster(parent_dir.firstCluster, content);
            found = TRUE;
        }
    }

    if (found == FALSE)
    	return ERROR;

    // free the FAT entries that the file used to use
    int fat_index;
    int cluster_to_delete = file.firstCluster;
    for (fat_index = 0; fat_index < file.clustersFileSize; fat_index++) {
        int tmp_cluster = local_fat[cluster_to_delete];
        if (set_value_to_fat(cluster_to_delete, FREE_CLUSTER) != SUCCESS)
        	return ERROR;
        cluster_to_delete = tmp_cluster;
    }

    // release resources for path
    free(path);

    return SUCCESS;
}

FILE2 open2 (char *filename) {
	// extract path head and tail
    Path *path = malloc(sizeof(Path));
    if (path_from_name(filename, path) != SUCCESS) {
        free(path);
        return ERROR;
    }

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

	if (file.TypeVal == TYPEVAL_LINK) //we must open the real file
	{
		Record* filefromlink = malloc(sizeof(Record));
		filefromlink = findLinkRecord(file);
		if (filefromlink == NULL)
			return ERROR;
		if (filefromlink->TypeVal != TYPEVAL_REGULAR)
			return ERROR;
		else
		{
			char linkpath[phys_cluster_size()];
			strcpy(linkpath, findLinkPath(file)->both);
			file = *filefromlink;
			free(filefromlink);
			return save_as_opened(file, linkpath);
		}
	}
	

    // release resources for path
    free(path);

	// if possible, save as opened
	// otherwise, returns a error
	return save_as_opened(file, filename);
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
	// Check if handle is inside of boundaries
	if (handle < 0)
		return ERROR;
	if (handle >= MAX_OPENED_FILES)
		return ERROR;
	// Check if the passed handle has a file 
	if (opened_files[handle].is_used == FALSE)
		return ERROR;

	// get the file from the opened list
	Record file = opened_files[handle].file; 
    
    int current_pointer = opened_files[handle].current_pointer;
	
	// creates a buffer to read the content
	unsigned char content[file.clustersFileSize * SECTOR_SIZE * superblock.SectorsPerCluster];
	
	// read cluster by cluster and saves on the buffer
	int read_index = file.firstCluster;
	int i;
	for (i = 0; i < file.clustersFileSize; i++) {
		if (read_cluster(read_index, &content[i * SECTOR_SIZE * superblock.SectorsPerCluster]) != SUCCESS) return ERROR;
		read_index = local_fat[read_index];
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
	// Check if handle is inside of boundaries
	if (handle < 0)
		return ERROR;
	
    if (handle >= MAX_OPENED_FILES)
		return ERROR;
	
    // Check if the passed handle has a file 
	if (opened_files[handle].is_used == FALSE)
		return ERROR;

	// get the file from the opened list
	Record file = opened_files[handle].file; 
	int current_pointer = opened_files[handle].current_pointer;
	int cluster_size = superblock.SectorsPerCluster * SECTOR_SIZE;
	int size_with_write = current_pointer + size;
	int content_size = cluster_size * file.clustersFileSize;
	int total_bytes = file.bytesFileSize;

	if (size_with_write > file.bytesFileSize) {
		content_size += size;
		
        // if this happens then total_bytes need to be updated
		total_bytes = size_with_write;
	}

	// total number of cluster = file size in bytes / cluster size in bytes
	// ceil is used because if a cluster has 1024bytes and a file has 1025,
	// then 2 clusters are necessary
	int file_total_clusters = ceil(((float) total_bytes) / cluster_size);
	
	// here we find out how many clusters need be allocated
	int file_clusters_to_alloc = file_total_clusters - file.clustersFileSize;

	// number of clusters really allocated
	int file_clusters_allocated = 0;

	if (file_clusters_to_alloc > 0) {
		int fat_last_index = file.firstCluster;
		while (local_fat[fat_last_index] != EOF)
			fat_last_index = local_fat[fat_last_index];
		int fat_index;
		for (fat_index = 0; fat_index < file_clusters_to_alloc; fat_index++) {
			int new_fit = phys_fat_first_fit();
			if (new_fit != ERROR) {
				if (set_value_to_fat(fat_last_index, new_fit) != SUCCESS)
					return ERROR;
				
                if (set_value_to_fat(new_fit, EOF) != SUCCESS)
					return ERROR;
				
                fat_last_index = new_fit;
				file_clusters_allocated++;
			} else {
				break;
			}
		}
		set_value_to_fat(fat_last_index, EOF);
	}

	// creates a buffer to read the content
	unsigned char content[content_size];
	
	// read cluster by cluster and saves on the buffer
	int read_index = file.firstCluster;
	int index;
	for (index = 0; index < file.clustersFileSize + file_clusters_allocated; index++) {
		if (read_cluster(read_index, &content[index * SECTOR_SIZE * superblock.SectorsPerCluster]) != SUCCESS) return ERROR;
		read_index = local_fat[read_index];
	}

	// update the content
	memcpy(&content[current_pointer], buffer, size);
	
	int write_index = file.firstCluster;
	for (index = 0; index < file.clustersFileSize; index++) {
		write_cluster(write_index, &content[index * SECTOR_SIZE * superblock.SectorsPerCluster]);
		write_index = local_fat[write_index];
	}

	// we need the path to get the parent folder
	Path *path = malloc(sizeof(Path));
    if (path_from_name(opened_files[handle].path, path) != SUCCESS) {
        free(path);
        return ERROR;
    }

	// we need the parent folder to update the record
	Record parent_dir;
    lookup_parent_descriptor_by_name(path->tail, &parent_dir);
	
	// Here we update the entry of the file on the parent directory
    // buffer to read the content of parent dir cluster
	unsigned char parent_content[SECTOR_SIZE * superblock.SectorsPerCluster];
    if (read_cluster(parent_dir.firstCluster, parent_content) != SUCCESS) return ERROR;

    int i;

    Record tmp_record;

    for (i = 0; i < records_per_sector() * superblock.SectorsPerCluster; i++) {
        int position_on_cluster = i * RECORD_SIZE;
        // Copy the current record do parent dir to tmp_record
        memcpy(&tmp_record, &parent_content[position_on_cluster], RECORD_SIZE);
        if (strcmp(tmp_record.name, file.name) == 0) {
        	// update the file size
            file.bytesFileSize = total_bytes;
            file.clustersFileSize = file.clustersFileSize + file_clusters_allocated;

            // save the updated file
            memcpy(&parent_content[position_on_cluster], &file, RECORD_SIZE);
            
            // write the file with differences 
            write_cluster(parent_dir.firstCluster, parent_content);
        }
    }

    // update the register on opened_files
    opened_files[handle].file = file;

    // release resources for path
    free(path);

    return size;
}

int seek2 (FILE2 handle, DWORD offset) {
	// Check if handle is inside of boundaries
	if (handle < 0)
		return ERROR;
	if (handle >= MAX_OPENED_FILES)
		return ERROR;
	// Check if the passed handle has a file 
	if (opened_files[handle].is_used == FALSE)
		return ERROR;

	// validate offset
	if (offset < 0 && offset != -1) 
		return ERROR;

	// update the current_pointer of the passed handle
	opened_files[handle].current_pointer = (offset == -1) ? opened_files[handle].file.bytesFileSize : offset; 

	return SUCCESS;
}

/**
 * Return author names.
 * 
 * returns - SUCCESS if possible FALSE otherwise. 
 **/
int identify2 (char *name, int size) {
    char *group = "Catarina Nogueira 00245534\nJoão Camargo 00274722\nArthur Balbao 00228702\n\0";
    
    if (size < strlen(group)) {
        return ERROR;
    }
    
    strncpy(name, group, size - 1);
    
    name[size] = '\0';
    
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
    if (path_from_name(pathname, path) != SUCCESS) {
        free(path);
        return ERROR;
    }

    // unable to locate parent path in disk or
    // current name exists in disk
    // then return an error
    if (!does_name_exists(path->tail)) {
        return ERROR;
    }

    // current name exists in disk so we should
    // return an error
    if (does_name_exists(path->both)) {
        return ERROR;
    }

    // find first free fat physical sector entry
    int p_free_sector = phys_fat_first_fit();
    
    // convert physical fat sector entry to logical fat sector entry
    int l_fat_free_sector = fat_phys_to_log(p_free_sector);

    // read from logical sector and fill up buffer
    can_read_write = read_sector(l_fat_free_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) {
        return ERROR;
    }

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
    can_read_write = read_sector(l_data_free_sector, buffer);

    // something bad happened, disk may be corrupted
    if (can_read_write != SUCCESS) return ERROR;
    
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

    // refresh local fat
    set_local_fat();

    // release resources for path
    free(path);

    return SUCCESS;
}

/**
 * Remove an existing directory. 
 *
 * param pathname - absolute or relative path for directory
 * 
 * returns - SUCCESS if sucessfully removed FALSE otherwise.
**/
int rmdir2 (char *pathname) {
    // extract path head and tail
    Path *path = malloc(sizeof(Path));
    if (path_from_name(pathname, path) != SUCCESS) {
        free(path);
        return ERROR;
    }

    // return error if parent path does not exists
    if (!does_name_exists(path->both)) {
        return ERROR;
    }

	int isLink = 0;

    // get the descriptor for parent dir
    Record parent_dir;
    lookup_parent_descriptor_by_name(path->tail, &parent_dir);

    // find the to-be-deleted child dir
    Record child_dir;    
    lookup_descriptor_by_name(parent_dir.firstCluster, path->head, &child_dir);  

	Record* temprec = NULL;


    // Check if this record is a trully directory
    if (child_dir.TypeVal != TYPEVAL_DIRETORIO) {
		if (child_dir.TypeVal == TYPEVAL_LINK)
		{
			temprec = malloc(sizeof(Record));
			temprec = findLinkRecord(child_dir);
			if (temprec == NULL)
				return ERROR;
			if (temprec->TypeVal != TYPEVAL_DIRETORIO)
				return ERROR;

			path = findLinkPath(child_dir);
			lookup_parent_descriptor_by_name(path->tail, &parent_dir);
			isLink = 1;
			child_dir = *temprec;
		}
		else
			return ERROR;
    }

    // allocate a buffer for storing temp child cluster content
    unsigned char content[SECTOR_SIZE * superblock.SectorsPerCluster];
    if (read_cluster(child_dir.firstCluster, content) != SUCCESS) return ERROR;

    // loop thourgh children directory to check if its empty or not
    // marking its members (only . and ..) as free entriess
    int i;
    Record tmp_record;
    for (i = 0; i < records_per_sector() * superblock.SectorsPerCluster; i++) {

        // calculate cluster position
        int position_on_cluster = i * RECORD_SIZE;

        // Copy the current record do parent dir to tmp_record
        memcpy(&tmp_record, &content[position_on_cluster], RECORD_SIZE);

        // check if current record is parent or local (. and ..) dir refs
        int is_parent_ref = strcmp(tmp_record.name, "..") == 0;
        int is_local_ref  = strcmp(tmp_record.name, ".")  == 0;

        // if not . or .. then we must abort since we cannot
        // remove non-empty folders
        if (!is_parent_ref && !is_local_ref && tmp_record.TypeVal != TYPEVAL_INVALIDO) {
            return ERROR;
        }

        // if we are looping through . and .. we must mark it as free entires now
        if (is_parent_ref || is_local_ref) {

            // mark record as free
            tmp_record.TypeVal = TYPEVAL_INVALIDO;

            // copy current record back to data buffer so we can write
            // it back to disk when done
            memcpy(&content[position_on_cluster], &tmp_record, RECORD_SIZE);
        }
    }

    // write back to disk all free entries
    write_cluster(child_dir.firstCluster, content);

    // read parent dir to temp buffer
    if (read_cluster(parent_dir.firstCluster, content) != SUCCESS) return ERROR;

    // find child entry in parent cluster and mark it as free
    for (i = 0; i < records_per_sector() * superblock.SectorsPerCluster; i++) {
        // calculate cluster position
        int position_on_cluster = i * RECORD_SIZE;

        // Copy the current record do parent dir to tmp_record
        memcpy(&tmp_record, &content[position_on_cluster], RECORD_SIZE);

        if (strcmp(tmp_record.name, child_dir.name) == 0) {
            tmp_record.TypeVal = TYPEVAL_INVALIDO;

            // copy current modified record back to buffer
            memcpy(&content[position_on_cluster], &tmp_record, RECORD_SIZE);
        }
    }

    // write back to disk all free entries
    write_cluster(parent_dir.firstCluster, content);

    // clear fat entry of child dir
    if (set_value_to_fat(child_dir.firstCluster, FREE_CLUSTER) != SUCCESS) {
        return ERROR;
    }

	if (isLink)
		free(temprec);

    return SUCCESS;
}

/**
 * Change current directory to pathname. 
 *
 * param pathname - absolute or relative path
 * 
 * returns - SUCCESS if directory was changed FALSE otherwise.
**/
int chdir2(char *pathname) {
	// if pathname is only slash then we dont need to worry
	// about extracting path we can just set curr_dir to
	// root dir cluster

	if (strcmp(pathname, "/") == 0) {
		curr_dir = cluster_to_log_sector(superblock.RootDirCluster);

		return SUCCESS;
	}

	// extract path head and tail
	Path *path = malloc(sizeof(Path));
	if (path_from_name(pathname, path) != SUCCESS) {
		free(path);
		return ERROR;
	}



	// check if path exist
	int valid = does_name_exists(path->both);

	// unable to locate parent path in disk or
	// current name exists in disk
	// then return an error
	if (!valid) {
		return ERROR;
	}

	// find parent directory by tail
	Record dir;
	lookup_parent_descriptor_by_name(path->tail, &dir);

	Record file;
	lookup_descriptor_by_name(dir.firstCluster, path->head, &file);

	if (file.TypeVal == TYPEVAL_LINK)
	{

		Path* pathfromlink = malloc(sizeof(Path));
		pathfromlink = findLinkPath(file);
		lookup_parent_descriptor_by_name(pathfromlink->both, &dir);
		free(pathfromlink);

		// set current directory to path already found
		curr_dir = cluster_to_log_sector(dir.firstCluster);

		// free path pointer from memmory 
		free(path);

		return SUCCESS;

	}

	else
	{
		// find parent directory by tail
		lookup_parent_descriptor_by_name(path->both, &dir);

		// set current directory to path already found
		curr_dir = cluster_to_log_sector(dir.firstCluster);

		// free path pointer from memmory 
		free(path);

		return SUCCESS;
	}



}





int getcwd2 (char *name, int size) {
    // allocate name array that we will return at end
    char curr_name[MAX_PATH_SIZE];

    // clear curr_name
    memset(curr_name, 0x00, MAX_PATH_SIZE);

    // get current directory cluster
    DWORD curr_cluster = curr_data_cluster();

    // allocate a buffer for storing temp child cluster content
    unsigned char content[SECTOR_SIZE * superblock.SectorsPerCluster];
    if (read_cluster(curr_cluster, content) != SUCCESS) return ERROR;
    
    // name must be equal or greater than 2 since we must fill it with
    // atleast a slash and a string terminator character '\0'
    if (size < 2) {
        return ERROR;
    }

    // we are in root directory then we can return slash
    if (curr_cluster == superblock.RootDirCluster) {
        name[0] = '/';
        name[1] = '\0';
        
        return SUCCESS;
    }

    // store current directory record
    Record tmp_dir;

    // retrieve current directory record and store in tmp_dir
    lookup_descriptor_by_cluster(curr_cluster, &tmp_dir);

    // prepend current directory name to curr_name
    str_prepend(curr_name, tmp_dir.name);
    str_prepend(curr_name, "/");

    // loop thourgh children directory to check if its empty or not
    // marking its members (only . and ..) as free entriess
    while (tmp_dir.firstCluster != superblock.RootDirCluster) {
        lookup_descriptor_by_name(tmp_dir.firstCluster, "..", &tmp_dir);

        // retrieve current directory record and store in tmp_dir
        lookup_descriptor_by_cluster(tmp_dir.firstCluster, &tmp_dir);

        // prepend current directory name to curr_name
        // and make sure to not append root reference
        if (strcmp(tmp_dir.name, ".") != 0) { 
            str_prepend(curr_name, tmp_dir.name);
            str_prepend(curr_name, "/");
        }
    }

    // store curr_name length
    int curr_name_len = strlen(curr_name);

    // curr_name legth cannot be greater than size parameter
    if (curr_name_len > size) {
        return ERROR;
    }

    // store curr_name in name parameter
    strncpy(name, curr_name, curr_name_len);

    return SUCCESS;
}

DIR2 opendir2 (char *pathname) {
	// extract path head and tail
	Path *path = malloc(sizeof(Path));
	path_from_name(pathname, path);

	// return error if parent path does not exists
	if (!does_name_exists(path->tail))
		// path does not exists
		return ERROR;

	// get the descriptor for parent folder
	Record parent_dir;
	lookup_parent_descriptor_by_name(path->tail, &parent_dir);

	// find the to-be-deleted file inside of parent dir
	Record dirdesc;
	if (!lookup_descriptor_by_name(parent_dir.firstCluster, path->head, &dirdesc))
		// file does not exists
		return ERROR;
	if (!(dirdesc.TypeVal == TYPEVAL_DIRETORIO || dirdesc.TypeVal == TYPEVAL_LINK))
	{
		// It's not a directory or a softlink to one
		free(path);
		return ERROR;

	}



	if (dirdesc.TypeVal == TYPEVAL_LINK) //we must open the real file
	{
		Record* filefromlink = malloc(sizeof(Record));
		filefromlink = findLinkRecord(dirdesc);
		if (filefromlink == NULL)
			return ERROR;

		if (filefromlink->TypeVal != TYPEVAL_DIRETORIO)
			return ERROR;
		else
		{
			char linkpath[phys_cluster_size()];
			strcpy(linkpath, findLinkPath(dirdesc)->both);
			dirdesc = *filefromlink;
			free(filefromlink);
			return save_as_opened_dir(dirdesc, linkpath);
		}
	}


	// if possible, save as opened dir
	// otherwise, returns an error
	return save_as_opened_dir(dirdesc, pathname);
}

int readdir2(DIR2 handle, DIRENT2 *dentry) {
	// Check if handle is inside of boundaries
	if (handle < 0)
		return ERROR;
	if (handle >= MAX_OPENED_DIRS)
		return ERROR;
	// Check if the passed handle has a dir
	if (opened_dirs[handle].is_used == FALSE)
		return ERROR;

	//get dir
	Record dir = opened_dirs[handle].record;



	const int bufferSize = phys_cluster_size();
	// creates a buffer to read the content
	unsigned char result[bufferSize];
	int address = opened_dirs[handle].current_pointer;
	
	struct t2fs_record descriptor;
	if (read_cluster(dir.firstCluster, result) != SUCCESS) return ERROR;
	if ((address >= (phys_cluster_size() - 64)) || address < 0) //if address is higher than the last possible entry, we must return an error.
		return ERROR;

	address = findValidEntry(dir, address); //find the next valid entry
	if (address == ERROR)
	{
		return ERROR;
	}
	
	memcpy(&descriptor, &result[address], sizeof(descriptor));

	if (descriptor.TypeVal == TYPEVAL_DIRETORIO || descriptor.TypeVal == TYPEVAL_LINK || descriptor.TypeVal == TYPEVAL_REGULAR)
		{
		dentry->fileSize = descriptor.bytesFileSize;
		dentry->fileType = descriptor.TypeVal; 
		strcpy(dentry->name, descriptor.name);
		address = findValidEntry(dir, address);
		opened_dirs[handle].current_pointer = address;
		return SUCCESS;
		}
	else
		return ERROR;
}

int closedir2 (DIR2 handle) {
		// Check if handle is inside of boundaries
		if (handle < 0)
			return ERROR;
		if (handle >= MAX_OPENED_DIRS)
			return ERROR;
		// Check if the passed handle has a dir
		if (opened_dirs[handle].is_used == FALSE)
			return ERROR;
		opened_dirs[handle].is_used = FALSE;
		unusedDirHandles++;
		num_opened_dirs--;
		return SUCCESS;
	}



int ln2(char *linkname, char *filename) {

	if (sizeof(filename) >= phys_cluster_size())//if path size bigger than a cluster, we must return an error.
		return ERROR;

	// extract path head and tail
	Path *linkpath = malloc(sizeof(Path));
	if (path_from_name(linkname, linkpath) != SUCCESS) {
		free(linkpath);
		return ERROR;
	}

	// extract path head and tail
	Path *filepath = malloc(sizeof(Path));
	if (path_from_name(filename, filepath) != SUCCESS) {
		free(filepath);
		return ERROR;
	}

	// return error if file does not exist.
	if (!does_name_exists(filepath->both))
		return ERROR;

	// return error if parent path does not exist
	if (!does_name_exists(linkpath->tail))
		return ERROR;

	// get the descriptor for parent folder
	Record parent_dir;
	lookup_parent_descriptor_by_name(linkpath->tail, &parent_dir);

	// find first free fat physical sector entry
	int p_free_sector = phys_fat_first_fit();

	Record link;
	link.TypeVal = TYPEVAL_LINK;
	link.bytesFileSize = phys_cluster_size();
	link.clustersFileSize = 1;
	strncpy(link.name, linkpath->head, FILE_NAME_SIZE);

	link.firstCluster = p_free_sector;


	unsigned char linkContent[link.bytesFileSize];

	memcpy(linkContent, filepath->both, link.bytesFileSize);


	// buffer to read the content of parent dir cluster
	unsigned char content[SECTOR_SIZE * superblock.SectorsPerCluster];

	if (read_cluster(parent_dir.firstCluster, content) != SUCCESS) return ERROR;

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
			if (strcmp(tmp_record.name, link.name) == 0)  //if file already exists, delete the content which belongs to the original
			{
				return ERROR;
			}
		
			// If it's invalid = it's free (ie, we can write on it)
		}
		else {
			// copy the content of file to the actual position on cluster
			memcpy(&content[position_on_cluster], &link, RECORD_SIZE);

			// write the modified cluster (with the new file) 
			write_cluster(parent_dir.firstCluster, content);

			// doesn't need to iterate anymore
			able_to_write = TRUE;
		
			break;
		}
	}



	// parent directory is full
	if (able_to_write == FALSE)
		return ERROR;

	write_cluster(link.firstCluster, linkContent);

	// sets EOF to the found free index of FAT
	if (set_value_to_fat(p_free_sector, EOF) != SUCCESS)
		return ERROR;

	// release resources for path
	free(linkpath);
	free(filepath);

	return SUCCESS;

}



int truncate2 (FILE2 handle) {
	// Check if handle is inside of boundaries
	if (handle < 0)
		return ERROR;
	if (handle >= MAX_OPENED_FILES)
		return ERROR;
	// Check if the passed handle has a file 
	if (opened_files[handle].is_used == FALSE)
		return ERROR;

	// get the file from the opened list
	Record file = opened_files[handle].file;
	int current_pointer = opened_files[handle].current_pointer;
	unsigned int cluster_size = phys_cluster_size();
	int newSize = current_pointer - 1;

	// total number of cluster = file size in bytes / cluster size in bytes
	int newFileClusters = ceil(((float)newSize) / cluster_size);
	
	if (current_pointer == 0)
	{ 
		newSize = 0;
		newFileClusters = 1;
	}

	// free the FAT entries that the file used to use
	int clusterCounter;
	int cluster_to_delete = file.firstCluster;
	for (clusterCounter = 0; clusterCounter < file.clustersFileSize; clusterCounter++) {
		int tmp_cluster = local_fat[cluster_to_delete];
		if(clusterCounter >= newFileClusters)
			if (set_value_to_fat(cluster_to_delete, FREE_CLUSTER) != SUCCESS)
				return ERROR;
			if(clusterCounter == newFileClusters -1)
				if (set_value_to_fat(cluster_to_delete, EOF) != SUCCESS)
					return ERROR;
		cluster_to_delete = tmp_cluster;
	}
	file.bytesFileSize = newSize;
	file.clustersFileSize = newFileClusters;

	
	Path *path = malloc(sizeof(Path));
	if (path_from_name(opened_files[handle].path, path) != SUCCESS) {
		free(path);
		return ERROR;
	}

	// get the descriptor for parent folder
	Record parent_dir;
	lookup_parent_descriptor_by_name(path->tail, &parent_dir);

	// Here we update the entry of the file on the parent directory
// buffer to read the content of parent dir cluster
	unsigned char parent_content[SECTOR_SIZE * superblock.SectorsPerCluster];
	if (read_cluster(parent_dir.firstCluster, parent_content) != SUCCESS) return ERROR;

	int i;

	Record tmp_record;

	for (i = 0; i < records_per_sector() * superblock.SectorsPerCluster; i++) {
		int position_on_cluster = i * RECORD_SIZE;
		// Copy the current record do parent dir to tmp_record
		memcpy(&tmp_record, &parent_content[position_on_cluster], RECORD_SIZE);
		if (strcmp(tmp_record.name, file.name) == 0) {
			// save the updated file
			memcpy(&parent_content[position_on_cluster], &file, RECORD_SIZE);

			// write the file with differences 
			write_cluster(parent_dir.firstCluster, parent_content);
		}
	}

    return SUCCESS;
}