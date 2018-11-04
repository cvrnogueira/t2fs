#include "../include/t2fs.h"
#include "../include/fs_helper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void attempt(int expected, char* msg, char* path) {
	printf("\n%s", msg);
	int t = create2(path);
	printf("\n\tResultado: %s", t == expected ? "SUCESSO!" : "ERRO!!!");
}

int main() {
	char* file_to_insert = "./filesabce.txt";
	attempt(0, "Teste para arquivo novo valido", file_to_insert);
	attempt(-1, "Teste para arquivo com path invalido", "./dummy/dummy.txt");
	attempt(-1, "Teste para arquivo repetido", file_to_insert);

	print_disk();

	return 0;
}
