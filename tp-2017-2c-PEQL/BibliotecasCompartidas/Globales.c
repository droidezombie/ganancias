/*
 * Globales.c
 *
 *  Created on: 7/9/2017
 *      Author: utnso
 */
#include "commons/string.h"
#include "commons/config.h"
#include "Globales.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>

int redondearHaciaArriba(int num,int num2){
	if(num%num2 ==0){
		return (int)num/num2;
	}
	else{
		return (int)(num/num2)+1;
	}
}

bool validarArchivo(char* path) {
	if (access(path, R_OK) == -1) {
		return 0;
	} else {
		return 1;
	}
}

void obtenerNumeroNodo(t_config* archivo,char* claveCopia,ubicacionBloque* ubi){

	char **arrayInfoBloque = config_get_array_value(archivo, claveCopia);
	int	numeroNodo = atoi(string_substring_from(arrayInfoBloque[0], 4));
	int bloqueNodo = atoi(string_substring_from(arrayInfoBloque[1], 0));

	ubi->numeroBloqueEnNodo = bloqueNodo;
	ubi->numeroNodo = numeroNodo;
}

void limpiarPantalla(){
	system("clear");
}

bool validarDirectorio(char* path){
	DIR* dir = opendir(path);
	if (dir)
	{
	    printf("Exite el directorio %s en el FileSystem\n", path);
	    return true;
	}
	else if (ENOENT == errno)
	{
	    printf("No existe el archivo %s en el FileSystem\n", path);
	    return false;
	}
	else
	{
	    return false;
	}
}

char* rutaSinArchivo(char* rutaArchivo){
	int index = 0, cantidadPartesRuta = 0;
	char* rutaInvertida = string_reverse(rutaArchivo);
	char* rutaFinal = malloc(string_length(rutaArchivo)+1);
	char* currentChar = malloc(2);
	char* nombreInvertido = malloc(strlen(rutaArchivo)+1);
	char** arrayPath = string_split(rutaArchivo, "/");

	while(arrayPath[cantidadPartesRuta] != NULL)
		++cantidadPartesRuta;

	if (cantidadPartesRuta == 1)
		return "yamafs:/";

	memset(nombreInvertido,0,string_length(rutaArchivo)+1);
	memset(rutaFinal,0,string_length(rutaArchivo)+1);
	memset(currentChar,0,2);

	currentChar = string_substring(rutaInvertida, index, 1);
	while(strcmp(currentChar,"/")){
		memcpy(nombreInvertido + index, currentChar, 1);
		++index;
		currentChar = string_substring(rutaInvertida, index, 1);
	}


	memcpy(rutaFinal, rutaArchivo, string_length(rutaArchivo)-index-1);

	//printf("ruta sin archivo %s\n", rutaFinal);
	if(strcmp(rutaFinal, "yamafs:") == 0)
		return "yamafs:/";

	//free(rutaFinal);
	free(currentChar);
	free(nombreInvertido);

	return rutaFinal;
}
