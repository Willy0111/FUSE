#include "fuseHeaders.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_time(time_t raw_time) {
    struct tm *timeinfo;
    char buffer[80];

    timeinfo = localtime(&raw_time);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    printf("%s\n", buffer);
}

int subdir_inmediato(const char* parent,const char* child) {
    size_t parent_len = strlen(parent);
    size_t child_len = strlen(child);
    
    if(parent_len >= child_len){
		return -1;
	}
    
    if(isPrefix(parent,child)==-1){
		return -1;
	}	
    
    int cont=0;
    int i;

	for(i=parent_len-1;  i<child_len; i++ ){
		if(child[i]=='/'){
			cont++;
			if(cont>1){
				return -1;
			}
		}
	}

    return 0;
}

// Nos devuelve el último bloque que esté libre
int nextEmptyBlock(){
    for(int i = 1; i < FILESYSTEM_SIZE; i++){
        if(strcmp(fs[i].path,"")==0){
            return i;
        }
    }
    return -1;
}

// Nos devuelve el último bloque ocupado (ha de tener next a -1)
int lastUsedBlock(){
    if(fs[0].siguiente==-1){
        return 0;
    }
    int actual = fs[0].siguiente;
    int next = fs[actual].siguiente;

    while((next)!=-1){
        actual = fs[actual].siguiente;
        next = fs[actual].siguiente;
    }

    if(next == -1 && actual != -1){
        return actual;
    }

    return -1;
}

char* buildFullPath(const char* filename) {
    if (strcmp(filename, ".") == 0) {
        char* retorno = malloc(strlen(currentDir->path) + 1);
        strcpy(retorno, currentDir->path);
        return retorno;
    }

    if (strcmp(filename, "..") == 0) {
        if (strcmp(currentDir->path, "/") == 0) {
            printf("Ya estás en / ...\n");
            return NULL;
        }

        int i = strlen(currentDir->path) - 1;
        while (i >= 0 && currentDir -> path[i] != '/') {
            i--;
        }
        
        char* retorno = malloc(sizeof(char) * (i + 1));
        if (retorno == NULL) {
            printf("Error al reservar memoria.\n");
            return NULL;
        }
        strncpy(retorno, currentDir->path, i);
        retorno[i] = '\0';
        return retorno;
    }

    if(strcmp(currentDir->path,"/")==0){//Si se da este caso, el path es directamente el parametro
        if(strlen(filename)>=LONGEST_FILENAME){
            printf("Tam maximo excedido");
            return NULL;
        }

        char* retorno = malloc(sizeof(char)*LONGEST_FILENAME);
        retorno[0] = '\0';
        strcpy(retorno,filename);
        return retorno;
    }

    unsigned int pathLen = strlen(currentDir->path);
    unsigned int filenameLen = strlen(filename);
    unsigned int size = pathLen + filenameLen + 2;

    if (size >= LONGEST_FILENAME) {
        printf("El nuevo nombre de fichero pasa de la cantidad especificada\n");
        return NULL;
    }

    char* newPath = (char*)malloc(size);
    if (newPath == NULL) {
        printf("Error al reservar memoria para el path.\n");
        return NULL;
    }

    strcpy(newPath, currentDir->path);

    if (currentDir->path[pathLen - 1] != '/') {
        strcat(newPath, "/");
    }
    
    strcat(newPath, filename);

    return newPath;
}

// Devuelve 0 si la cadena 1 es prefijo de la segunda. -1 en otro caso
int isPrefix(const char* prefix, const char* secondChain){
    if(strlen(prefix) > strlen(secondChain)){
        return -1;
    }
    int i;
    for(i = 0; i < strlen(prefix); i++){
        if(prefix[i] != secondChain[i]){
            return -1;
        }
    }
    return 0;
}

// Función para reemplazar el prefijo de `cadena` con `nuevo_prefijo`
void reemplazar_prefijo(char *cadena, const char *prefijo, const char *nuevo_prefijo) {
    // Verificar si `prefijo` es realmente un prefijo de `cadena`
    if (isPrefix(prefijo, cadena)==0) {
        // Calcular el tamaño de la parte de la cadena después del prefijo
        size_t tamano_prefijo = strlen(prefijo);
        size_t tamano_nuevo_prefijo = strlen(nuevo_prefijo);
        size_t tamano_restante = strlen(cadena) - tamano_prefijo;

        // Mover la parte restante de la cadena hacia adelante
        memmove(cadena + tamano_nuevo_prefijo, cadena + tamano_prefijo, tamano_restante + 1);

        // Copiar el nuevo prefijo en la posición correcta
        memcpy(cadena, nuevo_prefijo, tamano_nuevo_prefijo);
    }
}

char ultimoElemento(const char *cadena) {
    
    if (strcmp(cadena, "/") == 0) {
        return NULL;
    }

    int longitud = strlen(cadena);
    int i = longitud - 1;

    while (i >= 0 && cadena[i] != '/') {
        i--;
    }
    
    char* resultado = malloc(sizeof(char) * (longitud-i));
    if (resultado == NULL) {
        perror("No se pudo asignar memoria");
        return NULL;
    }
    resultado[0]='\0';
    strncpy(resultado, cadena + i +1, longitud - i);

    return resultado;
}

int exists(const char* absoluteFilename){
    int current = 0;
    while(fs[current].siguiente != -1 && strcmp(fs[current].path, absoluteFilename)!=0){
        current = fs[current].siguiente;
    }
    if(strcmp(fs[current].path, absoluteFilename)==0){
        return current;
    }
    return -1;
}

void printFileSystemState(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < FILESYSTEM_SIZE; i++) {
        if (fs[i].path[0] != '\0') { // Solo imprimir entradas válidas
            char creationTimeStr[20];
            strftime(creationTimeStr, sizeof(creationTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&fs[i].creation_time));
            fprintf(file, "Index: %d\nPath: %s\nSiguiente: %d\nCreation Time: %s\n\n",
                    i, fs[i].path, fs[i].siguiente, creationTimeStr);
        }
    }

    fclose(file);
}
