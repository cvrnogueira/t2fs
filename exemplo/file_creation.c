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

int ut_open(int expected, char* msg, char* path) {
	printf("\n%s", msg);
	int handle = open2(path);
	printf(" = %s", handle == expected ? "SUCESSO!" : "ERRO!!!");
	return handle;
}

void ut_close(int expected, char* msg, int handle) {
	printf("\n%s", msg);
	int t = close2(handle);
	printf(" = %s", t == expected ? "SUCESSO!" : "ERRO!!!");
}

void ut_read(int handle, int expected_size, char* msg, char* expected) {
	printf("\n%s", msg);
	char r[5];
	int sz = read2(handle, r, 5);
	//printf("\nABC = %i", strlen(expected));
	printf(" = %s", (strcmp(r, expected) == 0 && expected_size == sz) ? "SUCESSO!" : "ERRO!!!");
}

void write_block() {
	int handle = open2("file1.txt");
	char* content = "ola";
	write2(handle, content, 3);
	seek2(handle, 0);
	char content_read[5];
	read2(handle, content_read, 5);
	printf("\n%s", "Teste para escrever em um arquivo valido");
	printf(" = %s", strcmp(content_read, "olae ") == 0 ? "SUCESSO" : "ERRO");
	seek2(handle, 0);
	content = "Ess";
	write2(handle, content, 3);
	seek2(handle, 0);
	read2(handle, content_read, 5);
	printf("\n%s", "Teste para escrever em um arquivo valido");
	printf(" = %s", strcmp(content_read, "Esse ") == 0 ? "SUCESSO" : "ERRO");	
}

void test_block() {
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

	ut_close(0, "Teste para fechar um arquivo valido", 5);
	ut_close(0, "Teste para fechar um arquivo valido", 4);
	ut_open(4, "Teste para abrir um arquivo valido", filename);
	ut_close(0, "Teste para fechar um arquivo valido", 4);
	ut_close(0, "Teste para fechar um arquivo valido", 3);
	ut_close(0, "Teste para fechar um arquivo valido", 2);
	ut_close(0, "Teste para fechar um arquivo valido", 1);
	ut_close(0, "Teste para fechar um arquivo valido", 0);
	ut_close(-1, "Teste para fechar um arquivo invalido", -30);
	ut_close(-1, "Teste para fechar um arquivo invalido", 30);
	ut_close(-1, "Teste para fechar um arquivo invalido", 7);
	ut_close(-1, "Teste para fechar um arquivo invalido", 0);

	int handle = ut_open(0, "Teste para abrir um arquivo valido", "file1.txt");

	// ut_read le 5 caracteres por vez
	ut_read(handle, 5, "Teste para ler um arquivo valido", "Esse ");
	ut_read(handle, 5, "Teste para ler um arquivo valido", "eh o ");
	ut_read(handle, 5, "Teste para ler um arquivo valido", "arqui");

	seek2(handle, 0);
	ut_read(handle, 5, "Teste para ler um arquivo valido", "Esse ");
	ut_read(handle, 5, "Teste para ler um arquivo valido", "eh o ");

	seek2(handle, 0);
	ut_read(handle, 5, "Teste para ler um arquivo valido", "Esse ");

	seek2(handle, 10);
	ut_read(handle, 5, "Teste para ler um arquivo valido", "arqui");

	seek2(handle, -1);
	//ut_read(0, 5, "Teste para ler um arquivo valido", "\0");
	
	ut_close(handle, "Teste para fechar arquivo valido", 0);
	
	write_block();
	write_block();
	printf("\n");
}

int main() {
	
	test_block();
	test_block();
	
	/*
	int handle = open2("dir1/file1.txt");
	printf("\nRESULT = %i", handle);
	char content[200];
	printf("\nVOU DAR READ");
	int t = read2(handle, content, 5000);
	printf("\n%s", content);
	/*printf("\nVOU DAR WRITE");
	char *cnt_write = "c";
	seek2(handle, 0);
	write2(handle, cnt_write, 1);
	printf("\nVAU DAR READ");
	seek2(handle, 0);
	read2(handle, content, 100);
	printf("\n%s", content);
	delete2("file12.txt");*/
	printf("\n");
	return 0;
}
