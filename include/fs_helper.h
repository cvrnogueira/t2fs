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

//define max number of opened directories at the same time
#define MAX_OPENED_DIRS 65536

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

DWORD* local_fat;

BYTE buffer[SECTOR_SIZE];

typedef struct {
	char head[MAX_PATH_SIZE];
	char tail[MAX_PATH_SIZE];
	char both[MAX_PATH_SIZE];
} Path;

typedef struct {
	int is_used;
	int current_pointer;
    Record  file;
	char path[MAX_PATH_SIZE];
} OpenedFile;

typedef struct {
	int is_used;
	int current_pointer;
	Record  record;
	char* path;
} OpenedDir;


OpenedFile opened_files[MAX_OPENED_FILES];
OpenedDir opened_dirs[MAX_OPENED_DIRS];
int num_opened_files;
int num_opened_dirs;
int unusedDirHandles;
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
int lookup_descriptor_by_name(DWORD cluster, char *name, Record *record);

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
int lookup_parent_descriptor_by_name(char *name, Record *record);

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
DWORD curr_data_cluster(void);


/**
 * Lookup a record returning a contiguous logical position 
 * of found record accumulating positions from previous
 * sectors util a entry with a given type
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
DWORD lookup_cont_record_by_type(DWORD cluster, BYTE type);

/**
 * Lookup record descriptor by its cluster number.
 *
 * param cluster - logical cluster number
 * param record  - if matched then this variable will store record found during lookup
 *
 * returns - TRUE if found FALSE otherwise.
**/
int lookup_descriptor_by_cluster(DWORD cluster, Record *record);

/**
 * Converts a logical cluster number to sector number in data section.
 *
 * returns - sector number.
**/
DWORD cluster_to_log_sector(DWORD cluster);

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
 * 
 * on error - returns -1 if name does not exists or 0 if it exists.
**/
int path_from_name(char *name, Path *result);

/**
 * Prepend str2 in str1.
**/
void str_prepend(char *str1, char *str2);

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
DWORD fat_log_to_phys(DWORD lsector);

/**
 * Convert phyisical FAT sector entry to logical sector entry.
 * 
 * returns  - logical sector entry in FAT.
**/
DWORD fat_phys_to_log(DWORD psector);

/**
 * Finds first free physical entry in FAT.
 * 
 * returns  - physical entry index since first sector in FAT.
 * on error - returns -1 if cant read fat partition or theres no free entry.
**/

DWORD phys_fat_first_fit(void);

/*
 *  Refresh in-memmory fat table.
 *
 * This function is declared here because is not supposed 
 * to be accessed from outside.
*/
int set_local_fat();

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

/**
 * Read logical cluster and saves its content to the result buffer
 *
 * on error - returns ERROR if cant read from cluster otherwise SUCCESS.
**/
int read_cluster(int cluster, unsigned char *result);

/**
 * Writes content to logical cluster
 *
 * on error - returns ERROR if cant write to cluster otherwise SUCCESS.
**/
int write_cluster(int cluster, unsigned char *content);

/**
 * Save a record on the list of opened files
 *
 * Returns -1 on Error; index of the opened file on Success
**/
int save_as_opened(Record record, char* path);


/*
Similar to save_as_opened, only now returning a directory handler and having a higher limit of concurrent opened dirs
*/
int save_as_opened_dir(Record record, char* path);


/* Finds the next valid entry for a directory.
returns -1 on errors; returns valid next address if viable*/
int findValidEntry(Record record, int address);

/* Finds the path pointed by link*/
Path* findLinkPath(Record link);

/* Finds the record pointed by link*/
Record* findLinkRecord(Record link);

/**
 * Save a dword on a given position of local FAT
 * Also update the FAT position on disk according to the local FAT
 *
 * Returns the result of write_sector (to raise an error, if necessary)
**/
int set_value_to_fat(int position, DWORD value);

/**
 * Helper functions to print data, fat and super blocks from disk.
**/
void print_fat();
void print_disk();
void print_superblock();
