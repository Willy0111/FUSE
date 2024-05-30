#ifndef FUSEHEADERS_H
#define FUSEHEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define LONGESTPATHSIZE 100

typedef struct Files{
    unsigned long size;       // Tamaño de los datos binarios
    char* binario;               // Puntero a los datos binarios
} TFiles;

typedef struct tabla{
    char* path;
    //struct tm fechaCreacion;
    struct Files* data;
    struct tabla* next;
}elementoTabla;

extern elementoTabla* globalTable; //Si empieza a fallar cambiarlo por: elementoTabla *globalTable;
extern char* currentPath;

int initEmptyFilesystem();
int initFromBin(char*);
void cleanFileSystem();
void exitFileSystem();
void totalsize();
elementoTabla* pathExists(char*);
int createRawEntry(char*);
char* checksPrevios(char*);
int createDir(char*);
int subdir_inmediato(const char*,const char*);
char* ultimoComponente(char*);
char* ls();
int guardarDatos(char*, char* , int);
void pwd();
void remove_last_element();
void remove_last_elementArg(char*);
void changeDirectory(char*);
void copiarDesdeArchivo(const char*, char*);
int devolverArchivo(char*,char*);
void cambiarHijos(const char*, const char*);
char* absoluteFromRelative(const char*);
void renombrar(const char*,const char*);

#endif // FUSEHEADERS_H

/*
PATHLENGTH,PATH|PATHLENGTH,PATH|PATHLENGTH,PATH?
PATHLENGTH,PATH,SIZE,DATA|PATHLENGTH,PATH,SIZE,DATA|/0/0/0/0/0/0/0/0
*/