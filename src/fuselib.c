#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>

#include "fuseHeaders.h"
#include "fuselibUtilities.c"
#include "fuseinitexit.c"
#include "fuseIO.c"

extern elementoTabla* globalTable;
extern char* currentPath;
extern char* FUSEINITFILES;

int createDir(char* newDir){
    // Verificar errores previos
    char* msgError = checksPrevios(newDir);   
    if(msgError != NULL){
        printf("%s\n", msgError);
        return 1;
    }

    // Crear la ruta completa del nuevo directorio
    char* newString = malloc(sizeof(char)*(strlen(currentPath)+strlen(newDir)+2));
    if (newString == NULL) {
        perror("Error al asignar memoria para newString");
        return 1;
    }

    strcpy(newString, currentPath);
    strcat(newString, newDir);
    strcat(newString, "/");

    // Verificar si el directorio ya existe
    if (pathExists(newString) != NULL){
        printf("El elemento a crear ya existe\n");
        free(newString);
        return 1;
    }

    // Encontrar el último elemento en la tabla global
    elementoTabla* toAppend = globalTable;
    while (toAppend->next != NULL){
        toAppend = toAppend->next;
    }

    // Crear un nuevo elemento para el nuevo directorio
    toAppend->next = malloc(sizeof(elementoTabla));
    if (toAppend->next == NULL) {
        perror("Error al asignar memoria para nuevo elemento de la tabla");
        free(newString);
        return 1;
    }
    toAppend = toAppend->next;

    // Asignar la ruta al nuevo elemento
    toAppend->path = malloc(sizeof(char)*(strlen(newString)+1));
    if (toAppend->path == NULL) {
        perror("Error al asignar memoria para la ruta del nuevo elemento");
        free(toAppend->next);
        free(newString);
        return 1;
    }
    strcpy(toAppend->path, newString);
    free(newString);

    // Inicializar los otros campos del nuevo elemento
    toAppend->data = NULL;
    toAppend->next = NULL;

    return 0;
}

char* ls(){ 
    char* retorno = malloc(sizeof(char) * LONGESTPATHSIZE); // Reservar memoria para retorno
    
    if (retorno == NULL) {
        perror("Error al asignar memoria para retorno");
        return NULL;
    }

    retorno[0] = '\0'; // Inicializar retorno como una cadena vacía

    elementoTabla* copy = globalTable -> next; // No necesitas hacer un casting innecesario aquí

    while (copy != NULL) {
        if (strcmp(currentPath, copy -> path)!=0 && subdir_inmediato(currentPath, copy->path)) {
            strcat(retorno, ultimoComponente(copy->path));
            strcat(retorno, "   ");
        }
        copy = copy->next;
    }

    printf("%s\n", retorno);
    return retorno;
}

void pwd(){
    printf("%s\n",currentPath);
}

void changeDirectory(char* newDir){
    if(strcmp(newDir,"..")==0){
        if(strcmp(currentPath,"/")==0){
            printf("Ya esta en el directorio superior, no se puede subir.\n");
        }else{
            remove_last_element();
            printf("Directorio cambiado.\n");
        }
        return;
    }
    // Crear la ruta completa del nuevo directorio
    char* newString = malloc(sizeof(char)*(strlen(currentPath)+strlen(newDir)+2));
    newString[0]='\0';
    if (newString == NULL) {
        perror("Error al asignar memoria para newString");
        return;
    }

    strcpy(newString, currentPath);
    strcat(newString, newDir);
    strcat(newString, "/");

    elementoTabla* savePath = pathExists(newString);
    free(newString);
    if (savePath != NULL){
        strcpy(currentPath,savePath->path);
        printf("Directorio cambiado\n");
    }else{
        printf("El directorio no existe\n");
    }

}

/*
Se pueden dar 4 casos:
    from: fichero    to: fichero        -> cambiar nombre                               Este ya esta listo
    from: fichero    to: directorio     -> cambiar path                                 Este ya esta listo
    from: directorio to: fichero        -> cambiar nombre del directorio y sus hijos    Por hacer
    from: directorio to: directorio     -> cambiar directorio y todos sus hijos         Este ya esta listo

    Directorio es cuando y solo cuando:
    path termina en /       o 
    path es exactamente ..

    tengo que acordarme de usar la función: cambiarHijos() a ver si funciona.

*/
void renombrar(const char* from,const char* to){
    if(to[strlen(to)-1] != '/' && strcmp(to,"..") != 0){//Si se da este caso, to es fichero
        //Controlamos que no sea from: directorio to: fichero
        if(from[strlen(from)-1] == '/' && strcmp(from,"..") != 0){perror("No se puede from: directorio to: fichero.\n");return;}
        //Llegados aqui, suponemos que es from:fichero to:fichero
        char* aux = absoluteFromRelative(from);
        elementoTabla* copyPath = pathExists(aux);
        if(copyPath==NULL){
            perror("El fichero a cambiar de nombre no se ha podido encontrar.\n");
            return;
        }
        char* copiaTo = malloc(sizeof(char)*(strlen(to)+1));
        strcpy(copiaTo,to);
        char* errores = checksPrevios(copiaTo);//Para no pasarle const char* como argumento
        free(copiaTo);
        if(errores!=NULL){  //Si contiene caracteres prohibidos, abortamos
            perror(errores);
            return;
        }
        remove_last_elementArg(aux);
        if(strcmp(absoluteFromRelative(from),aux)==0){
            free(copyPath -> path);
            copyPath -> path = malloc(sizeof(char)*(strlen("/")+strlen(to)+1));
            if(copyPath -> path==NULL){
                perror("No se ha podido reservar memoria./n");
                free(aux);
                return;
            }
            copyPath -> path[0] = '\0';
            strcpy(copyPath -> path,"/");
            strcat(copyPath -> path,to);
            free(aux);
            return;
        }
        free(copyPath -> path);
        copyPath -> path = malloc(sizeof(char)*(strlen(aux)+strlen(to)+1));
        if(copyPath -> path==NULL){
            perror("No se ha podido reservar memoria./n"); free(aux);
        }
        strcpy(copyPath -> path,aux);
        free(aux);
        strcat(copyPath -> path,to);
        printf("Fichero renombrado, nueva ruta:%s\n",copyPath -> path);
        return;
    }
    else{//To es un directorio, implementar los casos correspondientes...
        if(from[strlen(from)-1] != '/' && strcmp(from,"..") != 0){      //from es fichero
            //Convertir to en absoluto, guardar from (si existe) con un nuevo path construido con absolute(to) + from
            char* fromCopy = absoluteFromRelative(from);
            elementoTabla* aMoverFrom = pathExists(fromCopy);
            if(aMoverFrom==NULL){
                free(fromCopy);
                printf("No se ha podido encontrar el elemento a mover.");
                return;
            }
            
            //Aqui sabemos que to es directorio y el fichero de from existe. Vemos si to existe también.
            char* toCopy = absoluteFromRelative(to);

            if(strcmp(toCopy,to)==0){
                free(toCopy);
                toCopy = strdup("/\0");
            }

            elementoTabla* aMoverTo = pathExists(toCopy);
            if(aMoverTo==NULL){
                free(fromCopy);
                free(toCopy);
                printf("No se ha podido encontrar el directorio a donde se quiere mover el elemento.\n");
                return;
            }

            //Aqui ambos existen, tenemos que hacer un nuevo path de la forma: toCopy+from
            free(aMoverFrom -> path);
            aMoverFrom -> path = malloc(sizeof(char)*(strlen(toCopy)+strlen(from)+1));
            aMoverFrom -> path[0] = '\0';
            strcpy(aMoverFrom -> path,toCopy);
            strcat(aMoverFrom -> path,from);
            //Listo, liberamos punteros y retornamos.
            free(fromCopy);
            free(toCopy);
            return;
        }
        else{//from: directorio to: directorio -> cambiar directorio y todos sus hijos
            char* fromCopy = absoluteFromRelative(from);
            elementoTabla* aMoverFrom = pathExists(fromCopy);
            if(aMoverFrom==NULL){
                free(fromCopy);
                printf("No se ha podido encontrar el elemento a mover.");
                return;
            }

            char* toCopy = absoluteFromRelative(to);
            if(strcmp(toCopy,to)==0){   //no se si este if hace falta.
                free(toCopy);
                toCopy = strdup("/\0");
            }

            elementoTabla* aMoverTo = pathExists(toCopy);
            if(aMoverTo==NULL){
                free(fromCopy);
                free(toCopy);
                printf("No se ha podido encontrar el directorio a donde se quiere mover el elemento.\n");
                return;
            }

            char* copiaHijos = strdup(toCopy);
            strcat(copiaHijos,from);
            cambiarHijos(fromCopy,copiaHijos);

            free(copiaHijos);
            free(aMoverFrom -> path);

            aMoverFrom -> path = malloc(sizeof(char)*(strlen(toCopy)+strlen(from)+1));

            strcpy(aMoverFrom -> path, toCopy);
            strcat(aMoverFrom -> path, from);
            return;
        }
    }
    return;
}

void renombrarGPT(const char* from, const char* to) {
    if (to[strlen(to) - 1] != '/' && strcmp(to, "..") != 0) { // Si se da este caso, to es fichero
        // Controlamos que no sea from: directorio to: fichero
        if (from[strlen(from) - 1] == '/' && strcmp(from, "..") != 0) {
            // Permitimos renombrar un directorio a un nombre de fichero, ajustando el nombre del directorio
            printf("Renombrando directorio a nombre de fichero.\n");
            char* aux = absoluteFromRelative(from);
            elementoTabla* copyPath = pathExists(aux);
            if (copyPath == NULL) {
                perror("El directorio a cambiar de nombre no se ha podido encontrar.\n");
                free(aux);
                return;
            }
            char* copiaTo = malloc(sizeof(char) * (strlen(to) + 1)); // +1 para el '\0'
            if (copiaTo == NULL) {
                perror("No se ha podido reservar memoria.\n");
                free(aux);
                return;
            }
            strcpy(copiaTo, to);
            char* errores = checksPrevios(copiaTo); // Para no pasarle const char* como argumento
            if (errores != NULL) { // Si contiene caracteres prohibidos, abortamos
                perror(errores);
                free(aux);
                free(copiaTo);
                return;
            }
            free(copiaTo); // Liberamos copiaTo después de pasar checksPrevios
            remove_last_elementArg(aux);
            if (strcmp(absoluteFromRelative(from), aux) == 0) {
                free(copyPath->path);
                copyPath->path = malloc(sizeof(char) * (strlen("/") + strlen(to) + 2)); // +2 para el '/' y el '\0'
                if (copyPath->path == NULL) {
                    perror("No se ha podido reservar memoria.\n");
                    free(aux);
                    return;
                }
                strcpy(copyPath->path, "/");
                strcat(copyPath->path, to);
                strcat(copyPath->path, "/");
                free(aux);
                return;
            }
            free(copyPath->path);
            copyPath->path = malloc(sizeof(char) * (strlen(aux) + strlen(to) + 2)); // +2 para el '/' y el '\0'
            if (copyPath->path == NULL) {
                perror("No se ha podido reservar memoria.\n");
                free(aux);
                return;
            }
            strcpy(copyPath->path, aux);
            strcat(copyPath->path, to);
            strcat(copyPath->path, "/");
            free(aux);
            printf("Directorio renombrado, nueva ruta: %s\n", copyPath->path);
            return;
        }
        // Llegados aquí, suponemos que es from: fichero to: fichero
        char* aux = absoluteFromRelative(from);
        elementoTabla* copyPath = pathExists(aux);
        if (copyPath == NULL) {
            perror("El fichero a cambiar de nombre no se ha podido encontrar.\n");
            free(aux);
            return;
        }
        char* copiaTo = malloc(sizeof(char) * (strlen(to) + 1));
        if (copiaTo == NULL) {
            perror("No se ha podido reservar memoria.\n");
            free(aux);
            return;
        }
        strcpy(copiaTo, to);
        char* errores = checksPrevios(copiaTo); // Para no pasarle const char* como argumento
        free(copiaTo);
        if (errores != NULL) { // Si contiene caracteres prohibidos, abortamos
            perror(errores);
            free(aux);
            return;
        }
        remove_last_elementArg(aux);
        if (strcmp(absoluteFromRelative(from), aux) == 0) {
            free(copyPath->path);
            copyPath->path = malloc(sizeof(char) * (strlen("/") + strlen(to) + 1));
            if (copyPath->path == NULL) {
                perror("No se ha podido reservar memoria.\n");
                free(aux);
                return;
            }
            strcpy(copyPath->path, "/");
            strcat(copyPath->path, to);
            free(aux);
            return;
        }
        free(copyPath->path);
        copyPath->path = malloc(sizeof(char) * (strlen(aux) + strlen(to) + 1));
        if (copyPath->path == NULL) {
            perror("No se ha podido reservar memoria.\n");
            free(aux);
            return;
        }
        strcpy(copyPath->path, aux);
        strcat(copyPath->path, to);
        free(aux);
        printf("Fichero renombrado, nueva ruta: %s\n", copyPath->path);
        return;
    } else { // To es un directorio, implementar los casos correspondientes...
        if (from[strlen(from) - 1] != '/' && strcmp(from, "..") != 0) { // from es fichero
            // Convertir to en absoluto, guardar from (si existe) con un nuevo path construido con absolute(to) + from
            char* fromCopy = absoluteFromRelative(from);
            elementoTabla* aMoverFrom = pathExists(fromCopy);
            if (aMoverFrom == NULL) {
                free(fromCopy);
                printf("No se ha podido encontrar el elemento a mover.\n");
                return;
            }
            // Aquí sabemos que to es directorio y el fichero de from existe. Vemos si to existe también.
            char* toCopy = absoluteFromRelative(to);
            if (strcmp(toCopy, to) == 0) {
                free(toCopy);
                toCopy = strdup("/\0");
                if (toCopy == NULL) {
                    perror("No se ha podido reservar memoria.\n");
                    free(fromCopy);
                    return;
                }
            }
            elementoTabla* aMoverTo = pathExists(toCopy);
            if (aMoverTo == NULL) {
                free(fromCopy);
                free(toCopy);
                printf("No se ha podido encontrar el directorio a donde se quiere mover el elemento.\n");
                return;
            }
            // Aquí ambos existen, tenemos que hacer un nuevo path de la forma: toCopy+from
            free(aMoverFrom->path);
            aMoverFrom->path = malloc(sizeof(char) * (strlen(toCopy) + strlen(from) + 1));
            if (aMoverFrom->path == NULL) {
                perror("No se ha podido reservar memoria.\n");
                free(fromCopy);
                free(toCopy);
                return;
            }
            strcpy(aMoverFrom->path, toCopy);
            strcat(aMoverFrom->path, from);
            // Listo, liberamos punteros y retornamos.
            free(fromCopy);
            free(toCopy);
            return;
        } else { // from: directorio to: directorio -> cambiar directorio y todos sus hijos
            char* fromCopy = absoluteFromRelative(from);
            elementoTabla* aMoverFrom = pathExists(fromCopy);
            if (aMoverFrom == NULL) {
                free(fromCopy);
                printf("No se ha podido encontrar el elemento a mover.\n");
                return;
            }
            char* toCopy = absoluteFromRelative(to);
            if (strcmp(toCopy, to) == 0) { // no se si este if hace falta.
                free(toCopy);
                toCopy = strdup("/\0");
                if (toCopy == NULL) {
                    perror("No se ha podido reservar memoria.\n");
                    free(fromCopy);
                    return;
                }
            }
            elementoTabla* aMoverTo = pathExists(toCopy);
            if (aMoverTo == NULL) {
                free(fromCopy);
                free(toCopy);
                printf("No se ha podido encontrar el directorio a donde se quiere mover el elemento.\n");
                return;
            }
            char* copiaHijos = malloc(sizeof(char) * (strlen(toCopy) + strlen(from) + 1));
            if (copiaHijos == NULL) {
                perror("No se ha podido reservar memoria.\n");
                free(fromCopy);
                free(toCopy);
                return;
            }
            strcpy(copiaHijos, toCopy);
            strcat(copiaHijos, from);
            cambiarHijos(fromCopy, copiaHijos);
            free(copiaHijos);
            free(aMoverFrom->path);
            aMoverFrom->path = malloc(sizeof(char) * (strlen(toCopy) + strlen(from) + 1));
            if (aMoverFrom->path == NULL) {
                perror("No se ha podido reservar memoria.\n");
                free(fromCopy);
                free(toCopy);
                return;
            }
            strcpy(aMoverFrom->path, toCopy);
            strcat(aMoverFrom->path, from);
            free(fromCopy);
            free(toCopy);
            return;
        }
    }
}

void rmfile(char* filename){
    elementoTabla* copia = (elementoTabla*) globalTable;
    char* newFilename = absoluteFromRelative(filename);

    while(copia -> next != NULL && strcmp(copia -> next -> path, newFilename)!= 0){
        copia = copia -> next;
    }

    free(newFilename);

    if(copia -> next == NULL){
        printf("No se ha podido encontrar el fichero");
        return;
    }

    elementoTabla* copiaDeLaCopia = (elementoTabla*) copia -> next;
    copia -> next = copiaDeLaCopia -> next;

    free(copiaDeLaCopia -> data -> binario);
    free(copiaDeLaCopia -> data);
    free(copiaDeLaCopia -> path);
    free(copiaDeLaCopia);

    printf("El fichero ha sido liberado\n");
}

//Dado un string buscar el directorio y borrarlo, borrando además todos sus hijos.
void removedir(char* filename){

    char* pathAbsoluto = absoluteFromRelative(filename);
    strcat(pathAbsoluto,"/");
    elementoTabla* copia = pathExists(pathAbsoluto);

    if(copia == NULL){
        printf("El directorio a buscar no se ha podido encontrar");
        return;
    }

    elementoTabla* aBorrar = (elementoTabla*) globalTable;
    
    while( aBorrar -> next != NULL ){
        if(startsWith(aBorrar->next->path,pathAbsoluto)==1){
            if(aBorrar->next->path[strlen(aBorrar->next->path)-1]!='/'){
                elementoTabla* copiaDeLaCopia = (elementoTabla*) aBorrar -> next;
                aBorrar -> next = copiaDeLaCopia -> next;

                free(copiaDeLaCopia->data->binario);
                free(copiaDeLaCopia->data);
                free(copiaDeLaCopia->path);
                free(copiaDeLaCopia);
            }else{
                elementoTabla* copiaDeLaCopia = (elementoTabla*) aBorrar -> next;
                aBorrar -> next = copiaDeLaCopia -> next;
                free(copiaDeLaCopia->path);
                free(copiaDeLaCopia);
            }
        }
        else{
            aBorrar = aBorrar -> next;
        }
    }

    free(pathAbsoluto);

    printf("Se ha borrado todo el contenido de la carpeta\n");
}