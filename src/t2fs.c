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
    return (0);
}

int delete2 (char *filename) {
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
