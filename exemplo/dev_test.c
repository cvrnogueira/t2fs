#include "t2fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
	//char *pathname;
	//pathname = malloc(100);

	print_disk();

	mkdir2("/folder_cat");

	return 0;
}
