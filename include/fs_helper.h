#include "t2fs.h"

/***************************************************************************
* definitions
***************************************************************************/

// define error and success constants
#define ERROR -1
#define SUCCESS 0

// define boolean values
#define TRUE 1
#define FALSE 0

// define file system version 2018/2
#define FS_VERSION 0x7E22

// define file system identifier
#define FS_ID "T2FS"

// define max number of open files in file system
#define MAX_OPENED_FILES 10

// defines a free cluster
#define FREE_CLUSTER 0x00000000

// defines an invalid cluster
#define INVALID_CLUSTER 0x00000001

// defines a cluster bad sector
#define BAD_SECTOR 0xFFFFFFFE

// defines an end o file marker
#define END_OF_FILE 0xFFFFFFFF

// defines fat entry size
#define FAT_ENTRY_SIZE 4

// defines file name max size
#define FILE_NAME_SIZE 52

// define record size in bytes
#define RECORD_SIZE sizeof(Record)

// define max chars in a path name
#define MAX_PATH_SIZE 4096


/***************************************************************************
* typedefs
***************************************************************************/

typedef struct t2fs_superbloco Superblock;

typedef struct t2fs_record Record;


/***************************************************************************
* global variables and structs
***************************************************************************/

Superblock superblock;

DWORD curr_dir;

BYTE buffer[SECTOR_SIZE];

typedef struct {
	char head[MAX_PATH_SIZE];
	char tail[MAX_PATH_SIZE];
} Path;


/***************************************************************************
* functions
***************************************************************************/

/**
 * Lookup Record descriptor by name in cluster.
 *
 * param cluster - logical cluster number
 * param name    - record name that this function will try to match
 * param record  - if matched then this variable will store record found during lookup
 *
 * returns - TRUE if found FALSE otherwise.
**/
int lookup_descriptor_by_name(int cluster, char *name, Record *record);

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
int lookup_parent_descriptor_by_name(char *name, Record *record)

/**
 * Number of records per sector.
 *
 * returns - number of records per sector.
**/
int records_per_sector(void);

/**
 * Calculates logical data cluster based on current directory pointer.
 *
 * returns - logical data cluster based on current directory.
**/
int curr_data_cluster(void);

/**
 * Converts a logical cluster number to sector number in data section.
 *
 * returns - sector number.
**/
int cluster_to_log_sector(int cluster);

/**
 * Check wheter a given name exists.
 *
 * returns - physical cluster size.
 * on error - returns -1 if name does not exists or 0 if it exists.
**/
int does_name_exists(char *name);

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
void path_from_name(char *name, Path *result);

/**
 * Physical cluster size calculated by sector per cluster from superblock.
 * 
 * returns  - physical cluster size.
**/
DWORD phys_cluster_size(void);

/**
 * Convert logical FAT sector entry to physical sector entry.
 * 
 * returns  - physical sector entry in FAT.
**/
int fat_log_to_phys(int lsector);

/**
 * Convert phyisical FAT sector entry to logical sector entry.
 * 
 * returns  - logical sector entry in FAT.
**/
int fat_phys_to_log(int psector);

/**
 * Finds first free physical entry in FAT.
 * 
 * returns  - physical entry index since first sector in FAT.
 * on error - returns -1 if cant read fat partition or theres no free entry.
**/

DWORD phys_fat_first_fit(void);

/**
 * Initialize curr_dir position to data sector after root sectors.
**/
int initialize_curr_dir(Superblock *superblock);

/**
 * Read superblock from sector zero
 * 
 * on error - return -1 if cant read superblock from sector zero
**/
int initialize_superblock(void);

// Fun��es para ajudar a visualizar o fs
void print_disk();
void print_superblock();
