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

int mkdir2 (char *pathname) {
    // extract path head and
    Path path;
    path = path_from_name(pathname);

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
    memcpy(&free_cluster, &eof, superblock.SectorsPerCluster);

    // write this sector back to disk in FAT
    write_sector(l_free_sector, buffer);

    // create current directory (head from path_from_name func)
    Record dir;
    dir.TypeVal = TYPEVAL_DIRETORIO;
    strcpy(dir.name, path.head);
    dir.bytesFileSize = phys_cluster_size();
    dir.firstCluster = p_free_sector;

    // TODO: 
    // - write directory to data sector
    // - create . and .. directories and write to data sector
    // - update parent directory
    // - missing '/' (root) checks

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
