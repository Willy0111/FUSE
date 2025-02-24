#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "fuseHeaders.h"
#include <fuse_common.h>

///////////////////////////////////////////////PROTOTIPOS DE FUNCIONES///////////////////////////////////////////////
static void *fs_init(struct fuse_conn_info *conn);
static int fs_statvfs(const char* restrict path, struct statvfs* restrict stbuf);
static int fs_getattr(const char *path, struct stat *stbuf);
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int fs_mkdir(const char *path, mode_t mode);
static int fs_rmdir(const char *path);
static int fs_rename(const char *from, const char *to);
static void fs_destroy(void *userdata);
static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int fs_unlink(const char *path);
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fs_open(const char *path, struct fuse_file_info *fi);
static int fs_release(const char *path, struct fuse_file_info *fi);
static int fs_getxattr(const char *path, const char *name, char *value, size_t size);
static int fs_truncate(const char *path, off_t newsize);

///////////////////////////////////////////////PROTOTIPOS DE FUNCIONES///////////////////////////////////////////////

///////////////////////////////////////FUNCIONES FUSE///////////////////////////////////////
static struct fuse_operations fs_oper = {
    .init    = fs_init,
    .statfs  = fs_statvfs,
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .mkdir   = fs_mkdir,
    .rmdir   = fs_rmdir,
    .rename  = fs_rename,
    .destroy = fs_destroy,
    .create  = fs_create,
    .unlink  = fs_unlink,
    .read    = fs_read,
    .write   = fs_write,
    .open    = fs_open,
    .release = fs_release,
    .getxattr= fs_getxattr,
    .truncate= fs_truncate,
};
///////////////////////////////////////FUNCIONES FUSE///////////////////////////////////////
static const char *fileSystemData = "fileSystem.bin";
/////////////////////////////////////IMPLEMENTACIÓN FUSE////////////////////////////////////
static void *fs_init(struct fuse_conn_info *conn) {
    if (conn) {
        conn->max_write = BLOCKSIZE; 
    } else {
        printf("fuse_conn_info is NULL\n");
    }
    return NULL;
}

static int fs_statvfs(const char* restrict path, struct statvfs* restrict stbuf){
	stbuf->f_bsize  = BLOCKSIZE;  					// Tamaño de bloque
    stbuf->f_frsize = BLOCKSIZE; 					// Tamaño de fragmento
    stbuf->f_blocks = (fsblkcnt_t) DATASYSTEM_SIZE; // Total de bloques
    stbuf->f_bfree  = (fsblkcnt_t) bloqueslibres(); // Bloques libres
    stbuf->f_bavail = (fsblkcnt_t) bloqueslibres(); // Bloques disponibles para usuarios no privilegiados
    stbuf->f_files 	= (fsfilcnt_t)FILESYSTEM_SIZE;  // Total de inodos
    stbuf->f_ffree 	= (fsfilcnt_t)nodoslibres();    // Inodos libres
    stbuf->f_favail = (fsfilcnt_t) nodoslibres();   // Inodos disponibles para usuarios no privilegiados
    stbuf->f_namemax= LONGEST_FILENAME;             // Máximo número de caracteres en un nombre de archivo

    return 0;
}

// Función para obtener atributos de un archivo o directorio
static int fs_getattr(const char *path, struct stat *stbuf) {
    printf("fs_getattr: Path = %s\n", path);
    memset(stbuf, 0, sizeof(struct stat));
	char* fullpath = buildFullPath(path);
	int idx= exists(fullpath);
	free(fullpath);
	printf("Index: %i\n", idx);
	
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = fs[0].mode;
        stbuf->st_nlink = fs[0].nlink + 2;
        stbuf->st_atime = fs[0].last_access;
        stbuf->st_mtime = fs[0].last_modification;
        stbuf->st_ctime = fs[0].creation_time;
		stbuf->st_uid	= fs[0].uid;
		stbuf->st_gid	= fs[0].gid;
		stbuf->st_size  = sizeof(FileSystemInfo);
		stbuf->st_blocks= 0;
    } else if((idx != -1) && (fs[idx].hasData == -1)){
		stbuf->st_mode = fs[idx].mode;
        stbuf->st_nlink = fs[idx].nlink + 2;
        stbuf->st_atime = fs[idx].last_access;
        stbuf->st_mtime = fs[idx].last_modification;
        stbuf->st_ctime = fs[idx].creation_time;
		stbuf->st_uid= fs[idx].uid;
		stbuf->st_gid= fs[idx].gid;
		stbuf->st_size  = sizeof(FileSystemInfo);
		stbuf->st_blocks= 0;
	} else if ((idx != -1) && (fs[idx].hasData != -1)){
		stbuf->st_mode = fs[idx].mode;
        stbuf->st_nlink = 1;
        stbuf->st_atime = fs[idx].last_access;
        stbuf->st_mtime = fs[idx].last_modification;
        stbuf->st_ctime = fs[idx].creation_time;
		stbuf->st_uid   = fs[idx].uid;
		stbuf->st_gid   = fs[idx].gid;
		stbuf->st_size  = ds[fs[idx].hasData].totalSize;
		stbuf->st_blocks= ((ds[fs[idx].hasData].totalSize)/BLOCKSIZE) + 1;
	} else {
		return -ENOENT;
	} 

    return 0;
}

// Función para leer un directorio
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
	(void) fi;
	char* fullpath = buildFullPath(path);
	int idx= exists(fullpath);
	free(fullpath);
	
	if(idx==-1){
		printf("No se encuentra");
		return -ENOENT;
	}
	
	if (filler(buf, ".", NULL, 0) != 0) {
		return -ENOMEM;
	}
	
	if(filler(buf, "..", NULL, 0) != 0){
		return -ENOMEM;
	}
	
    if (strcmp(path, "/") == 0) {
        // Añadir todas las entradas
        for (int i = 1; i < FILESYSTEM_SIZE; i++) {
            if (subdir_inmediato(path, fs[i].path)==0) { // Revisa si la entrada es válida
				char resultado[LONGEST_FILENAME];
				ultimoElemento(fs[i].path, resultado);
                if (filler(buf, resultado, NULL, 0) != 0) {
                    return -ENOMEM;
                }
            }
        }
    } else if(fs[idx].hasData == -1){
        for (int i=0; i < FILESYSTEM_SIZE; i++){
            if (subdir_inmediato(path, fs[i].path)==0) { // Revisa si la entrada es válida
				char resultado[LONGEST_FILENAME];
				ultimoElemento(fs[i].path, resultado);
                if (filler(buf, resultado, NULL, 0) != 0) {
                    return -ENOMEM;
                }
            }
		}
	} 
    return 0;
}

// Función para crear un directorio
static int fs_mkdir(const char *path, mode_t mode) {
    int result = createDir(path);
    if (result == -1) {
        printf("fs_mkdir: Failed to create directory.\n");
        return -EPERM;
    }
    return 0;
}

// Función para eliminar un directorio
static int fs_rmdir(const char *path) {
    printf("fs_rmdir: Path = %s\n", path);
    deleteElement(path);
    return 0;
}

static int fs_rename(const char* from, const char* to) {
    printf("fs_rename debug: from %s, to %s",from,to);
    //Entendemos que rename ya hace las comprobaciones de archivo
    if(strlen(to)>=LONGEST_FILENAME){
        return -ENAMETOOLONG;
    }
    int idx = exists(from);
    if(idx==-1){
        //Comprobación redundante, no necesaria en teoria
        return -ENOENT;
    }
    // Si hay que actualizar hijos es Directorio
    if(fs[idx].hasData==-1){
        int i=fs[0].siguiente;
        int futureSize = 0;
        int actualizarHijos = 1;
        while(actualizarHijos && i != -1){
            if(isPrefix(from,fs[i].path)==0){
                futureSize = strlen(fs[i].path)-strlen(from)+strlen(to);
                if(futureSize>=LONGEST_FILENAME){
                    actualizarHijos = 0;
                }
            }
            i = fs[i].siguiente;
        }
        //Si todos los hijos entran en el tamano apropiado, los actualizamos cuando sean prefijo.
        if(actualizarHijos){
			actualizar_padre(1,to);
			actualizar_padre(0,from);
            i = fs[0].siguiente;
            while(i != -1){
                reemplazar_prefijo(fs[i].path,from,to);
                i = fs[i].siguiente;
            }
            return 0;
        }
        else{
            return -ENAMETOOLONG;
        }
    }
    //Si no, es porque es un fichero.
    else{
        strcpy(fs[idx].path,to);
        actualizar_padre(1,to);
        actualizar_padre(0,from);
        return 0;
    }
}

// Función para destruir el sistema de archivos
static void fs_destroy(void *userdata) {
    printf("fs_destroy: Destroying file system\n");
    FileSystemInfo *fs = (FileSystemInfo*)userdata;
    printFileSystemState("salida");
    cleanup(fs, FILESYSTEM_SIZE * sizeof(FileSystemInfo), fileno(fopen(fileSystemData, "r")));
    printf("fs_destroy: File system destroyed\n");
}

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    int result = createFile(path, "", mode);
    if (result == -1) {
        printf("fs_create: Failed to create file.\n");
        return -EPERM;
    }
    return 0;
}

static int fs_unlink(const char *path){
    printf("fs_unlink: Path = %s\n", path);
    deleteElement(path);
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	printf("fs_read: Path = %s\n", path);
	char* fullpath = buildFullPath(path);
	int idx = exists(fullpath);
	free(fullpath);
	if(idx==-1){
		printf("fs_read: File not found.\n");
        return -ENOENT;	
	}
	
	if(fs[idx].hasData==-1){
		printf("fs_read: Not a file.\n");
        return -EISDIR;
	}
	
	if(ds[fs[idx].hasData].firstDataBlock != fs[idx].hasData){
		return -EIO;
	}
    size_t tam = sizeOfFile(fs[idx].hasData);
    if(offset < tam){
		if((offset + size) > tam){
			size = tam - offset;
		}
		char* temp = cat(fs[idx].hasData);
		memcpy(buf,temp, strlen(temp));
		free(temp);
	} else{
		size=0;
	}
    return size;
}

static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    //Para dd solo aceptamos bloques de 4096
	printf("fs_write: Path = %s\n", path);
	
	char* fullpath = buildFullPath(path);
	int idx = exists(fullpath);
	free(fullpath);
	if(idx==-1){
		printf("fs_write: File not found.\n");
        return -ENOENT;	
	}
    printf("IDX: %i\n", fs[idx].hasData);
    if(fs[idx].hasData==-1){
		printf("fs_write: Not a file.\n");
        return -EISDIR;
	}
    
	//Vienen varios bloques (ej dd, suponemos que la entrada está bien y la anexionamos:)
	if(offset != 0){
        if(copiarSinCheck(fs[idx].hasData,buf,size)==0){
            return size;
        }
        return 0;
	}
	
    if(borrarFile(fs[idx].hasData)==-1){
        return -EIO;
    }

    fs[idx].last_access = time(0);          
    fs[idx].last_modification = time(0);
	
	FILE* archivo=fopen(buf, "rb");
	if(archivo!=NULL){
		fseek(archivo, 0, SEEK_END);
		int tamano=ftell(archivo);
		fclose(archivo);
		fs[idx].hasData=insertData(buf);
		return tamano;
	}
	
    fs[idx].hasData = escribirDesdeBuffer(buf,size);
    return size;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
	char* fullpath = buildFullPath(path);
	int idx= exists(fullpath);
	free(fullpath);
	if(idx==-1){
		printf("fs_open: File not found.\n");
        return -ENOENT;	
	}
	if(fs[idx].hasData==-1){
		printf("fs_open: Not data.\n");
        return -EISDIR;
	}
	if (fi->flags & O_TRUNC) {
        if (truncate(path, 0) == -1) {
            close(fd);
            return -errno;
        }
    }
		
    fi->fh = idx;  // Guardamos file descriptor en fi
    return 0;
}

static int fs_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);  // Cerramos archivo
    return 0;
}

static int fs_getxattr(const char *path, const char *name, char *value, size_t size) {
    return -ENOTSUP;  // No tenemos atributos extendidos
}

//Implementada esta función para controlar algunos operadores, sin embargo, la lógica está ya implementada en el write
static int fs_truncate(const char *path, off_t newsize){
	char* fullpath= buildFullPath(path);
	int idx= exists(fullpath);
	if(idx == -1){
		return -ENOENT;
	}
	free(fullpath);
	return 0;
}
/////////////////////////////////////IMPLEMENTACIÓN FUSE///////////////////////////////////
int main(int argc, char *argv[]){
    // Verificar el número mínimo de argumentos y que los últimos dos no sean opciones (comienzan con '-')
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) {
        fprintf(stderr, "Uso incorrecto de los parámetros. Debe ser: ./programa <archivo_datos> <punto_montaje>\n");
        return 1;
    }
	init(argv[argc-2]);
    init_datasystem("dataSystem.bin");
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    return fuse_main(argc, argv, &fs_oper, fs);
}
