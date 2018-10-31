#include "t2fs.h"

#define ERROR 1
#define SUCCESS 0

#define FS_VERSION 0x7E22
#define FS_ID "T2FS"
#define MAX_OPENED_FILES 10
#define FREE_CLUSTER 0x00000000
#define INVALID_CLUSTER 0x00000001
#define BAD_SECTOR 0xFFFFFFFE
#define END_OF_FILE 0xFFFFFFFF
#define FAT_ENTRY_SIZE 4

typedef struct t2fs_superbloco Superblock;

Superblock superblock;
DWORD curr_dir;
BYTE buffer[SECTOR_SIZE];

typedef struct {
  char *head;
  char *tail;
} Path;

Path path_from_name(char *name);

DWORD first_fit(void);

int initialize_curr_dir(Superblock *superblock);
int initialize_superblock(void);

int is_error(int code);
int is_success(int code);

// Funções para ajudar a visualizar o fs
void print_disk();
void print_superblock();
