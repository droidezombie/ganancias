/*
 * Comandos.h
 *
 *  Created on: 7/9/2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_COMANDOS_H_
#define FILESYSTEM_COMANDOS_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include "Sockets.h"
#include <commons/log.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ctype.h>

int formatearFS(int flag);//format

int eliminarArchivo(char* comando);//rm

int eliminarDirectorio(char* comando);//rm -d

int eliminarBloque(char* comando);//rm -b

int cambiarNombre(char* comando);//rename

int mover(char* comando);//mv

int mostrarArchivo(char* comando);//cat

int crearDirectorio(char* comando);//mkdir

int copiarArchivo(char* comando);//cpfrom

int copiarArchivoAFs(char* comando);//cpto

int copiarBloqueANodo(char* comando);//cpblock

int generarArchivoMD5(char* comando);//md5

int listarArchivos(char* comando);//ls

int informacion(char* comando);//info

#endif /* FILESYSTEM_COMANDOS_H_ */
