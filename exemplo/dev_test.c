#include "t2fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
	printf("\n\n");

	printf("\033[22;34m==============================================================================\n");

	// printing test header warning in blue
	printf("EXECUTING DEVELOPMENT TESTS\n[TEST OUTPUT ERROR = 1 SUCCESS = 0]\n");
	
	printf("==============================================================================\n");

	int has_errors = 0; 

	// clears color buffer
	printf("\033[0m");

	/*has_errors += mkdir2("folder_cat/pablo");*/
	has_errors += chdir2("folder_cat");
	has_errors += chdir2("../dir1");
	has_errors += chdir2("..");
	has_errors += chdir2("./dir1");
	has_errors += chdir2("..");

	printf("\n\n");

	// print test success in green errors in red
	if (has_errors == 0) { printf("\033[22;32mTEST EXECUTION SUCCESS\n"); }
	else { printf("\033[22;31mTEST EXECUTION FAILED\n"); }

	// clears color buffer
	printf("\033[0m\n\n");

	return 0;
}
