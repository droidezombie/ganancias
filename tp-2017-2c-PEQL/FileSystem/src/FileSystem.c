/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "Sockets.h"
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include "Comandos.h"
#include <Configuracion.h>
#include <commons/bitarray.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "FuncionesFS.h"
#include <pthread.h>
#include <sys/types.h>
#include "FuncionesHilosFs.h"

#define cantDataNodes 10

int cantBloques = 10;
int sizeBloque = 1048576; // 1mb
int mostrarLoggerPorPantalla = 1;

char* rutaBitmaps = "../metadata/Bitmaps/";
char* rutaArchivos = "../metadata/Archivos/";
char* root = "../metadata/Archivos/0";

int cantidadDirectorios = 100;
int numeroCopiasBloque = 2;
//t_bitarray* bitmap[cantDataNodes];
t_log* loggerFS;
int sizeTotalNodos = 0, nodosLibres = 0;
t_list* nodosConectados;
t_list* bitmapsNodos;
sem_t pedidoFS;
t_list* pedidosFS;
extern sem_t actualizarNodos;
sem_t pedidoTerminado;
int clienteYama;
int servidorFS;
pthread_mutex_t logger_mutex;
pthread_mutex_t listSemMutex;
int EstadoFS = 0;
int bloquesLibresTotales = 0;
bool recuperarEstado = 0;
bool fsFormateado = false;

struct configuracionFileSystem config;

int main(int argc, char *argv[]) {

	limpiarPantalla();
	cargarConfiguracionFileSystem(&config,argv[1]);

	if (argv[2] != NULL)
		if (strcmp(argv[2],"--clean") == 0 && validarArchivo("../metadata/Nodos.bin")){
			system("rm ../metadata/Nodos.bin");
		}

	sem_init(&pedidoFS,0,0);
	sem_init(&pedidoTerminado,0,0);
	sem_init(&actualizarNodos,1,0);
	nodosConectados = list_create();
	bitmapsNodos = list_create();
	pedidosFS = list_create();

	pthread_mutex_init(&logger_mutex, NULL);
	pthread_mutex_init(&listSemMutex, NULL);
	pthread_t hiloServidorFS, hiloConsolaFS, hiloConexionYama;
	parametrosServidorHilo parametrosServidorFS;
	loggerFS = log_create("logFileSystem", "FileSystem.c", mostrarLoggerPorPantalla, LOG_LEVEL_TRACE);

	parametrosServidorFS.cliente = clienteYama;

	verificarEstadoAnterior();

	if (recuperarEstado == 1)
		inicializarTablaDirectorios();
	else{
		borrarDirectorios();

		borrarArchivosEnMetadata();

		liberarNodosConectados();

		mkdir("../metadata/Archivos/0",0777);
	}

	servidorFS = crearSocket();

	establecerServidor(config.IP_FILESYSTEM, config.PUERTO_FS);

	parametrosServidorFS.servidor = servidorFS;

	pthread_create(&hiloServidorFS,NULL,levantarServidorFS ,(void*)&parametrosServidorFS);
	pthread_create(&hiloConsolaFS,NULL,consolaFS ,NULL);

	pthread_join(hiloServidorFS, NULL);
	pthread_join(hiloConsolaFS, NULL);

	return 0;

}

