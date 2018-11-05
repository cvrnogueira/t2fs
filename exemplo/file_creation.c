#include "../include/t2fs.h"
#include "../include/fs_helper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void ut_create(int expected, char* msg, char* path) {
	printf("\n%s", msg);
	int t = create2(path);
	printf(" =  %s", t == expected ? "SUCESSO!" : "ERRO!!!");
}

void ut_delete(int expected, char* msg, char* path) {
	printf("\n%s", msg);
	int t = delete2(path);
	printf(" =  %s", t == expected ? "SUCESSO!" : "ERRO!!!");
}

void ut_open(int expected, char* msg, char* path) {
	printf("\n%s", msg);
	int t = open2(path);
	printf(" = %s", t == expected ? "SUCESSO!" : "ERRO!!!");
}

int main() {

	char* filename = "xxx.txt";
	delete2(filename);

	ut_create(0, "Teste para criar arquivo novo valido", filename);
	ut_create(-1, "Teste para criar arquivo com path invalido", "./dummy/dummy.txt");
	ut_create(-1, "Teste para criar arquivo repetido", filename);
	
	ut_delete(0, "Teste para deletar arquivo valido", filename);
	ut_create(-1, "Teste para deletar arquivo com path invalido", "./dummy/dummy.txt");
	ut_delete(-1, "Teste para deletar arquivo nao existente", filename);

	ut_create(1, "Teste para criar arquivo novo valido", "tmp1.txt");
	ut_create(2, "Teste para criar arquivo novo valido", "tmp2.txt");
	ut_delete(0, "Teste para deletar arquivo valido", "tmp1.txt");
	ut_delete(0, "Teste para deletar arquivo valido", "tmp2.txt");

	ut_create(3, "Teste para criar arquivo novo valido", filename);
	ut_open(4, "Teste para abrir um arquivo valido", filename);
	ut_open(5, "Teste para abrir um arquivo valido", filename);
	ut_open(-1, "Teste para abrir um arquivo invalido", "tmp12.txt");
	ut_open(-1, "Teste para abrir um arquivo com path invalido", "aa/xxx.txt");

	printf("\n");
	return 0;
}
