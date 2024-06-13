#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <assert.h>

#include "fuseHeaders.h"
#include "fileSystemUtils.c"

extern FileSystemInfo* currentDir;

void initialize_filesystem(FileSystemInfo *fs) {
    // Primero inicializamos root
    fs[0].hasData = -1;
    fs[0].path[0]='\0';
    strcpy(fs[0].path, "/");
    fs[0].siguiente = -1;
    fs[0].creation_time = time(0);
    fs[0].last_access = time(0);
    fs[0].last_modification = time(0);
    fs[0].uid = getuid();
    fs[0].gid = getgid();
    fs[0].mode = 0644;
    fs[0].nlink = 1;

    for (int i = 1; i < FILESYSTEM_SIZE; i++) {
        fs[i].hasData = -1;
        strcpy(fs[i].path,"\0");
        fs[i].siguiente = -1;
        fs[i].creation_time = time(NULL);
        fs[i].last_access = time(NULL);
        fs[i].last_modification = time(NULL);
        fs[i].uid = getuid();
        fs[i].gid = getgid();
        fs[i].mode = 0644;
        fs[i].nlink = 0;
    }
}

void init(const char *filename, FileSystemInfo **fs, size_t *filesize, int *fd, struct stat *st) {
    *fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (*fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (fstat(*fd, st) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    *filesize = FILESYSTEM_SIZE * sizeof(FileSystemInfo);
    if (st->st_size != *filesize) {
        if (ftruncate(*fd, *filesize) == -1) {
            perror("ftruncate");
            exit(EXIT_FAILURE);
        }
    }

    *fs = mmap(NULL, *filesize, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if (*fs == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if (st->st_size == 0) {
        initialize_filesystem(*fs);
    }

    currentDir = fs[0];
}

void cleanup(FileSystemInfo *fs, size_t filesize, int fd) {
    if (msync(fs, filesize, MS_SYNC) == -1) {
        perror("msync");
        exit(EXIT_FAILURE);
    }

    if (munmap(fs, filesize) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    close(fd);
}

void changeDirectory(FileSystemInfo *fs, const char* newDir){
    int directorioACambiar;

    char* fullPathString = buildFullPathDir(newDir);
    if(fullPathString==NULL){
        return;
    }

    directorioACambiar = exists(fs, fullPathString);

    //Si ya existía, la funcion devuelve -1, si no, devuelve el numero donde se encuentra.
    if(directorioACambiar==-1){
        printf("Path doesn't exist.\n");
        return;
    }

    currentDir = &fs[directorioACambiar];

    free(fullPathString);
}

int createDir(FileSystemInfo* fs, const char* filename){
    int nextEmptyBlock;
    int lastBlock;

    if(strchr(filename,'/')!=NULL){
        printf ("Illegal character in the dir to build.\n");
        return -1;
    }

    char* fullPathString = buildFullPathDir(filename);
    if(fullPathString==NULL){
        return -1;
    }

    //Si ya existía, la funcion devuelve su path, si no, -1.
    if(exists(fs, fullPathString)!=-1){
        printf("Path already exists.\n");
        free(fullPathString);
        return -1;
    }

    nextEmptyBlock = nextEmtpyBlock(fs);
    if(nextEmptyBlock==-1){
        printf("File system is full.\n");
        free(fullPathString);
        return -1;
    }

    lastBlock = lastUsedBlock(fs);
    assert(fs[lastBlock].siguiente == -1);    //Nos aseguramos de que mire a -1

    fs[lastBlock].siguiente = nextEmptyBlock;

    fs[nextEmptyBlock].hasData = -1;
    strcpy(fs[nextEmptyBlock].path, fullPathString);
    fs[nextEmptyBlock].siguiente = -1;
    fs[nextEmptyBlock].creation_time = time(0);
    fs[nextEmptyBlock].last_access = time(0);
    fs[nextEmptyBlock].last_modification = time(0);
    fs[nextEmptyBlock].uid = getuid();
    fs[nextEmptyBlock].gid = getgid();
    fs[nextEmptyBlock].mode = 0644;
    fs[nextEmptyBlock].nlink = 1;

    free(fullPathString);
    
    return 0;
}

//FUNCION INTERNA, SOLO SE PUEDE LLAMAR DESDE REMOVEDIR O REMOVEFILE
void borrar (FileSystemInfo* fs,char* absolutePath){
    int actual = 0;
    int posterior = fs[0].siguiente;

    while(posterior != -1 && strcmp(fs[posterior].path,absolutePath)!=0){
        actual = posterior;
        posterior = fs[posterior].siguiente;
    }

    if (posterior == -1) {
        printf("Path not found.\n");
        return;
    }

    //Ahora debemos borrar posterior. Ponemos actual.siguiente mirando a posterior.siguiente y
    //borramos los datos de posterior (como una linkedlist)
    fs[actual].siguiente = fs[posterior].siguiente;
    
    //Primero miramos si es un fichero para borrar todo su contenido asociado.
    if(fs[posterior].hasData!=-1){
        //Limpiar fichero asociado a fs[posterior].hasData
    }

    //Ahora borramos todo el contenido...
    memset(fs[posterior].path, 0, LONGEST_FILENAME);
    fs[posterior].siguiente = -1;
    fs[posterior].creation_time = time(NULL);
    fs[posterior].last_access = time(NULL);
    fs[posterior].last_modification = time(NULL);
    fs[posterior].uid = getuid();
    fs[posterior].gid = getgid();
    fs[posterior].mode = 0644;
    fs[posterior].nlink = 0;
}

void removeDir(FileSystemInfo* fs, const char* filename){
    char* fullPathString = buildFullPathDir(filename);
    
    if(fullPathString==NULL){
        return;
    }

    if(strcmp(filename,"/")==0){
        printf("Cannot remove root directory.\n");
        free(fullPathString);
        return;
    }

    int saveExist = exists(fs, fullPathString);
    if(saveExist == -1){
        printf("Directory doesn't exist.\n");
        free(fullPathString);
        return;
    }

    int elemento = 0;
    
    for (int i = 0; i < FILESYSTEM_SIZE; i++) {
        if (strlen(fs[i].path) != 0 && isPrefix(fullPathString, fs[i].path)==0) {
            printf("Se va a borrar: %s\n",fs[i].path);
            borrar(fs, fs[i].path);
        }
    }

    free(fullPathString);
    printf("Directory and its content has been removed succesfully.\n");
}

//Implementada para fichero -> fichero y fichero -> directorio
//me falta el directorio -> lo que sea.
//y falta testearla...
int renameItem(FileSystemInfo* fs, const char* oldName, const char* newName) {
    char* absOldName = buildFullPathGeneral(oldName);
    if(absOldName==NULL){
        printf("No se ha podido reservar memoria");
        return 1;
    }
    int copiaExists = exists(fs,absOldName);
    if(copiaExists==-1){
        printf("No existe el elemento a renombrar...\n");
        free(absOldName);
        return 1;
    }

    if(oldName[strlen(oldName)-1]=='/'){
        //Como oldname es un directorio, lleve newName / al final o no, será un renombrado
        //tener en cuenta los hijos
    }
    else{
        //oldName es un fichero, miramos si la operaciónes es mover o cambiar nombre
        if(newName[strlen(newName)-1]=='/'||strcmp(newName,".")==0||strcmp(newName,"..")==0){
            //newName es un directorio, tenemos que moverlo
            //Tenemos que comprobar que el directorio exista pero no exista con el archivo dentro
            char* newDir = buildFullPathGeneral(newName);
            if(exists(fs,newDir)==-1){
                printf("No existe el directorio donde guardar el elemento...\n");
                free(absOldName);
                free(newDir);
                return 1;
            }
            //En este caso, el nuevo nombre será newDir+oldName.
            strcat(newDir,oldName);
            //Miramos que no exista el archivo dentro.
            if(strlen(newDir) >= LONGEST_FILENAME-1 || exists(fs,newDir)!=-1){
                printf("Ya existe un archivo con ese nombre...\n");
                free(absOldName);
                free(newDir);
                return 1;
            }
            //Hechas todas estas comprobaciones, ya solo editamos el nombre.
            strcpy(fs[copiaExists].path,newDir);
            free(absOldName);
            free(newDir);
            return 0;
        }else{
            //renombramos... Como estamos trabajando en relativo, el nuevo path será currentdir+newName
            //comprobamos que newName tenga un formato válido:
            if(strchr(newName,'/')!=NULL){
                printf("Formato de renombre inválido, no puede contener el caracter: /\n");
                free(absOldName);
                return -1;
            }
            if(strlen(currentDir -> path)+strlen(newName)+1 >= LONGEST_FILENAME){
                printf("Path demasiado largo...\n");
                free(absOldName);
                return -1;
            }
            strcpy(fs[copiaExists].path,currentDir -> path);
            strcat(fs[copiaExists].path,newName);

            return 0;
        }
    }
    return 1;
}


int main() {
    const char *filename = "filesystem.bin";
    FileSystemInfo* fs;
    size_t filesize;
    int fd;
    struct stat st;

    init(filename, &fs, &filesize, &fd, &st);

    removeDir(fs,"JuanDir");

    printFileSystemState(fs,"temp.txt");
    cleanup(fs, filesize, fd);

    return 0;
}
