#include "../include/t2fs.h"
#include "../include/fs_helper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void ut_create(int expected, char* msg, char* path) {
	printf("\n%s", msg);
	int t = create2(path);
	printf("\n\tResultado: %s", t == expected ? "SUCESSO!" : "ERRO!!!");
}

void ut_delete(int expected, char* msg, char* path) {
	printf("\n%s", msg);
	int t = delete2(path);
	printf("\n\tResultado: %s", t == expected ? "SUCESSO!" : "ERRO!!!");
}

int main() {

	char* file_to_insert = "xxx.txt";

	ut_create(0, "Teste para criar arquivo novo valido", file_to_insert);
	ut_create(-1, "Teste para criar arquivo com path invalido", "./dummy/dummy.txt");
	ut_create(-1, "Teste para criar arquivo repetido", file_to_insert);
	
	ut_delete(0, "Teste para deletar arquivo valido", file_to_insert);
	ut_create(-1, "Teste para deletar arquivo com path invalido", "./dummy/dummy.txt");
	ut_delete(-1, "Teste para deletar arquivo nao existente", file_to_insert);
	
	printf("\n");
	return 0;
}
