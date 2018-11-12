#include "t2fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// define max size of name
#define NAME_SIZE 4096

int main() {
	printf("\n\n");

	// printing test header warning in blue
	printf("\033[22;34m==============================================================================\n");

	printf("EXECUTING DEVELOPMENT TESTS\n[TEST OUTPUT ERROR = 1 SUCCESS = 0]\n");
	
	printf("==============================================================================\n\n");

	// flag to check number of errors during test execution
	int has_errors = 0; 

	// clears color buffer
	printf("\033[0m");

	// make sure dir5 exists doesnt matter what
	has_errors += mkdir2("dir5");
	has_errors += mkdir2("dir5/dir6");
	has_errors += mkdir2("dir5/dir6/dir7");
	has_errors += mkdir2("dir5/dir6/dir7/dir8");
	has_errors = 0;

	// create and remove same dir
	has_errors += mkdir2("dir3");
	has_errors += rmdir2("dir3");

	// create a dir and a subdir
	has_errors += mkdir2("dir4");
	has_errors += mkdir2("dir4/dir5");

	// try to remove a non-empty dir and sum 1 to its result since
	// it will be an error
	has_errors += rmdir2("dir4") + 1;

	// delete children and parent to let disk be on its normal
	// state again
	has_errors += rmdir2("dir4/dir5");
	has_errors += rmdir2("dir4");

	// change current directory to dir1
	has_errors += chdir2("dir1");

	// go back to root and change current dir to dir5
	has_errors += chdir2("../dir5");

	// go back to root 
	has_errors += chdir2("..");

	// alocate name to use in getcwd2
	char* name = malloc(sizeof(char) * NAME_SIZE);

	// get current working directory as string
	has_errors += getcwd2(name, NAME_SIZE);
	
	// since we are in root dir make sure getcwd2 returned / as its result
	has_errors += strcmp(name, "/");

	// change to ./dir5/dir6/dir7/dir8
	has_errors += chdir2("./dir5/dir6/dir7/dir8");

	// clear name
	memset(name, 0x00, NAME_SIZE);

	// get current working directory as string
	has_errors += getcwd2(name, NAME_SIZE);
	
	// make sure getcwd2 matches with current directory
	has_errors += strcmp(name, "/dir5/dir6/dir7/dir8");

	// clear name
	memset(name, 0x00, NAME_SIZE);

	// change to root
	has_errors += chdir2("/");

	// get current working directory as string
	has_errors += getcwd2(name, NAME_SIZE);

	// make sure getcwd2 matches with current directory
	has_errors += strcmp(name, "/");

	// clear name
	memset(name, 0x00, NAME_SIZE);

	// change to dir6
	has_errors += chdir2("dir5/dir6");

	// get current working directory as string
	has_errors += getcwd2(name, NAME_SIZE);

	// make sure getcwd2 matches with current directory
	has_errors += strcmp(name, "/dir5/dir6");

	printf("\n\n");

	// print test success in green errors in red
	if (has_errors == 0) { printf("\033[22;32mTEST EXECUTION SUCCESS\n"); }
	else { printf("\033[22;31mTEST EXECUTION FAILED\n"); }

	// clears color buffer
	printf("\033[0m\n\n");

	free(name);

	return 0;
}
