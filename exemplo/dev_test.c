#include "t2fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
	char *pathname;
	pathname = malloc(100);

	mkdir2("/dir1/jujubs");

	return 0;
}