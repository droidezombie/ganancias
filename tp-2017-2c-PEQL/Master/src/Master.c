/*
 * Master.c
 *
 *  Created on: 26/10/2017
 *      Author: utnso
 */

#include "FuncionesMaster.h"

struct configuracionMaster config;

int main(int argc, char *argv[]) {
	limpiarPantalla();

	if(argc < 5) {
		puts("Faltan argumentos");
		return EXIT_FAILURE;
	}

	loggerMaster = log_create("logMaster", "Master.c", 1, LOG_LEVEL_TRACE);
	pthread_mutex_init(&mutexReplanificar,NULL);

	cargarConfiguracionMaster(&config,argv[1]);
	conectarseConYama(config.YAMA_IP,config.YAMA_PUERTO);
	miJob = crearJob(argv);
	enviarJobAYama(miJob);
	esperarInstruccionesDeYama();

	return EXIT_SUCCESS;
}
