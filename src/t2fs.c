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
    print_superblock();
    print_disk();
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
