#ifndef FUSEHEADERS_H
#define FUSEHEADERS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LONGEST_FILENAME 64  // Tamaño de path más largo permitido
#define FILESYSTEM_SIZE 1024 // Número de entradas en el sistema de archivos

#define DATASYSTEM_SIZE 1024 // Número de entradas en el sistema de datos
#define BLOCKSIZE 128       // Numero de bytes por bloque

typedef struct info {
    char path[LONGEST_FILENAME];
    int siguiente;
    time_t creation_time;
    time_t last_access;             
    time_t last_modification;
    uid_t uid;
    gid_t gid;
    mode_t mode;
    int nlink;
    int hasData;
} FileSystemInfo;

extern FileSystemInfo* fs;
extern size_t filesize;
extern int fd;
extern struct stat st;

extern FileSystemInfo* currentDir;

typedef struct data{
	int firstDataBlock;
    int currentBlockSize;
	unsigned long totalSize;
	char dat[BLOCKSIZE];
	int siguiente;
} DataSystemInfo;

extern DataSystemInfo *ds;
extern size_t dataFilesize;
extern int dataFd;
extern struct stat dataSt;

// Declaraciones de funciones de fileSystemLib.c
void initialize_filesystem();
void init(const char *);
void cleanup();
void changeDirectory(const char*);
int createDir(const char*);
void deleteElement(const char*);
//int renameItem(FileSystemInfo *fs, const char* oldName, const char* newName);
void borrar(const char*);
int createFile(const char*, const char*);
// Declaraciones de funciones de fileSystemLib.c


// Declaraciones de funciones de fileSystemUtils.c
int exists(const char*);
void print_time(time_t);
int nextEmptyBlock();
int lastUsedBlock();
char* buildFullPath(const char*);
int isPrefix(const char*, const char*);
void printFileSystemState(const char *);
int subdir_inmediato(const char*,const char*);
void ultimoElemento(const char*, const char*);
void actualizar_padre(int);
// Declaraciones de funciones de fileSystemUtils.c


// Declaraciones de funciones de dataSystemLib.c
void initialize_datasystem();
void init_datasystem(const char*);
int primerElementoLibre();
int hayEspacio(int);
int copiarFichero(int, FILE*, long , int);
int insertData(const char*);
char* cat(int);
void escribirArchivoBinario(const char*, int, size_t);
int borrarFile(int);
size_t sizeOfFile(int);
int escribirDesdeBuffer(const char*);
int copiarStream(int , const char*, long , int);
// Declaraciones de funciones de dataSystemLib.c

#endif
