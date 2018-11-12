#include "../include/t2fs.h"
#include "../include/fs_helper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int ut_create(int expected, char* msg, char* path) {
	printf("\n%s", msg);
	int t = create2(path);
	printf(" =  %s", t == expected ? "SUCESSO!" : "ERRO!!!");
	return t;
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
	printf(" = %s", (strcmp(r, expected) == 0 && expected_size == sz) ? "SUCESSO!" : "ERRO!!!");
}

void write_block_1() {
	int handle = open2("file1.txt");
	char* content = "ola";
	write2(handle, content, 3);
	seek2(handle, 0);
	char content_read[5];
	read2(handle, content_read, 5);
	printf("\n%s", "Teste para escrever em um arquivo valido");
	printf(" = %s", strcmp(content_read, "olae ") == 0 ? "SUCESSO!" : "ERRO");
	seek2(handle, 0);
	content = "Ess";
	write2(handle, content, 3);
	seek2(handle, 0);
	read2(handle, content_read, 5);
	printf("\n%s", "Teste para escrever em um arquivo valido");
	printf(" = %s", strcmp(content_read, "Esse ") == 0 ? "SUCESSO!" : "ERRO");	
	close2(handle);
}

void write_block_aux(char* file, int N) {
	int handle = open2(file);
	char cnt_write[N+1];
	memset(cnt_write, 'X', N);
	cnt_write[N] = '\0';
	//printf("\nEU INICIO = %s", cnt_write);
	seek2(handle, 0);
	int res = write2(handle, cnt_write, N);
	seek2(handle, 0);
	char content_read[N+1];
	read2(handle, content_read, N);
	content_read[N] = '\0';
	//printf("\nVOLTOU = %s", content_read);
	//printf("\nEU ESPERO = %s", cnt_write);
	printf("\n%s", "Teste para escrever em um arquivo valido");
	printf(" = %s", strcmp(content_read, cnt_write) == 0 ? "SUCESSO!" : "ERRO!!!");
	//printf("\ncnt_write tem %i\n", sizeof(cnt_write));
	close2(handle);
}

void write_block_2(char* file) {
	int i;
	for (i = 0; i < 15; i++) {
		int N = (2^i) + 1;
		write_block_aux(file, N);
	}
	
	for (i = 0; i < 15; i++) {
		int N = (2^i) + 1;
		write_block_aux(file, N);
	}
	printf("\n");
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
	
	write_block_1();
	write_block_2("file2.txt");

	handle = ut_create(0, "Teste para criar arquivo novo valido", "foo");
	write_block_2("foo");
	close2(handle);
	delete2("foo");
	handle = ut_create(0, "Teste para criar arquivo novo valido", "dir1/foo");
	write_block_2("dir1/foo");
	close2(handle);
	delete2("dir1/foo");

	printf("\n");
}

int main() {
	
	test_block();
	test_block();

	/*print_fat();

	int h = open2("file1.txt");
	write_block_aux("file1.txt", 2049);

	delete2("file1.txt");
	print_fat();*/
	return 0;
}
