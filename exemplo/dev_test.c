#include "t2fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
	printf("\n\n");

	// printing test header warning in blue
	printf("\033[22;34m==============================================================================\n");

	printf("EXECUTING DEVELOPMENT TESTS\n[TEST OUTPUT ERROR = 1 SUCCESS = 0]\n");
	
	printf("==============================================================================\n");

	// flag to check number of errors during test execution
	int has_errors = 0; 

	// clears color buffer
	printf("\033[0m");

	// make sure dir5 exists doesnt matter what
	has_errors += mkdir2("dir5");
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

	printf("\n\n");

	// print test success in green errors in red
	if (has_errors == 0) { printf("\033[22;32mTEST EXECUTION SUCCESS\n"); }
	else { printf("\033[22;31mTEST EXECUTION FAILED\n"); }

	// clears color buffer
	printf("\033[0m\n\n");

	return 0;
}
