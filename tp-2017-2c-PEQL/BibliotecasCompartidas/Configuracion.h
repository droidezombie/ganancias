/*
 * Configuracion.h
 *
 *  Created on: 7/9/2017
 *      Author: utnso
 */

#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct configuracionYama{
	int FS_PUERTO;
	char* FS_IP;
	int YAMA_PUERTO;
	char* YAMA_IP;
	int RETARDO_PLANIFICACION;
	char* ALGORITMO_BALANCEO;
	int DISPONIBILIDAD_BASE;
}configuracionYama;

struct configuracionNodo{
	int PUERTO_FILESYSTEM;
	char* IP_FILESYSTEM;
	char* IP_NODO;
	int PUERTO_WORKER;
	char* NOMBRE_NODO;
	int PUERTO_DATANODE;
	char* RUTA_DATABIN;
	int SIZE_NODO;
}configuracionNodo;

struct configuracionMaster{
	char* YAMA_IP;
	int YAMA_PUERTO;
}configuracionMaster;

struct configuracionFileSystem{
	char* IP_FILESYSTEM;
	int PUERTO_FS;
}configuracionFileSystem;

void cargarConfiguracionYama(struct configuracionYama *config,char* rutaAConfig);

void cargarConfiguracionMaster(struct configuracionMaster *config,char* rutaAConfig);

void cargarConfiguracionNodo(struct configuracionNodo *config,char* rutaAConfig);

void cargarConfiguracionFileSystem(struct configuracionFileSystem *config, char* rutaAConfig);

char* obtenerRutaRealConfiguracion(char* rutaAConfig);

#endif /* CONFIGURACION_H_ */
