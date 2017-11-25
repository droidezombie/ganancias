/*
 * Configuracion.c
 *
 *  Created on: 7/9/2017
 *      Author: utnso
 */

#include "Configuracion.h"
#include <commons/config.h>
#include <string.h>
#include <unistd.h>

//Configuracion de YAMA

void cargarConfiguracionYama(struct configuracionYama *config,char* rutaAConfig){
	char* pathArchConfig = obtenerRutaRealConfiguracion(rutaAConfig);

	t_config* configYama = config_create(pathArchConfig);

	if (config_has_property(configYama, "FS_IP")){
		config->FS_IP= config_get_string_value(configYama,"FS_IP");
		printf("La IP del FileSystem es %s \n",config->FS_IP);
	}

	if (config_has_property(configYama, "FS_PUERTO")){
		config->FS_PUERTO = config_get_int_value(configYama,"FS_PUERTO");
		printf("El puerto del FileSystem es: %d \n",config->FS_PUERTO);
	}
	if (config_has_property(configYama, "YAMA_IP")){
		config->YAMA_IP= config_get_string_value(configYama,"YAMA_IP");
		printf("La IP del YAMA es %s \n",config->YAMA_IP);
	}

	if (config_has_property(configYama, "YAMA_PUERTO")){
		config->YAMA_PUERTO = config_get_int_value(configYama,"YAMA_PUERTO");
		printf("El puerto de YAMA es: %d \n",config->YAMA_PUERTO);
	}

	if (config_has_property(configYama, "RETARDO_PLANIFICACION")){
		config->RETARDO_PLANIFICACION = config_get_int_value(configYama,"RETARDO_PLANIFICACION");
		printf("El retardo de planificacion es: %d \n",config->RETARDO_PLANIFICACION);
	}

	if (config_has_property(configYama, "ALGORITMO_BALANCEO")){
		config->ALGORITMO_BALANCEO = config_get_string_value(configYama,"ALGORITMO_BALANCEO");
		printf("El algoritmo de balanceo es %s \n",config->ALGORITMO_BALANCEO);
	}
	if (config_has_property(configYama, "DISPONIBILIDAD_BASE")){
			config->DISPONIBILIDAD_BASE = config_get_int_value(configYama,"DISPONIBILIDAD_BASE");
			printf("La disponibilidad base es: %d \n",config->DISPONIBILIDAD_BASE);
		}
}

//Configuracion de Master

void cargarConfiguracionMaster(struct configuracionMaster *config,char* rutaAConfig){
	char* pathArchConfig = obtenerRutaRealConfiguracion(rutaAConfig);

	t_config* configMaster = config_create(pathArchConfig);

	if (config_has_property(configMaster, "YAMA_IP")){
		config->YAMA_IP= config_get_string_value(configMaster,"YAMA_IP");
		printf("La IP del YAMA es %s \n",config->YAMA_IP);
	}

	if (config_has_property(configMaster, "YAMA_PUERTO")){
		config->YAMA_PUERTO = config_get_int_value(configMaster,"YAMA_PUERTO");
		printf("El puerto de YAMA es: %d \n",config->YAMA_PUERTO);
	}
}

void cargarConfiguracionNodo(struct configuracionNodo *config,char* rutaAConfig){
	char* pathArchConfig = obtenerRutaRealConfiguracion(rutaAConfig);

	t_config* configNodo = config_create(pathArchConfig);

	if (config_has_property(configNodo, "IP_FILESYSTEM")){
		config->IP_FILESYSTEM= config_get_string_value(configNodo,"IP_FILESYSTEM");
		printf("La IP del FileSystem es %s \n",config->IP_FILESYSTEM);
	}

	if (config_has_property(configNodo, "PUERTO_FILESYSTEM")){
		config->PUERTO_FILESYSTEM= config_get_int_value(configNodo,"PUERTO_FILESYSTEM");
		printf("El puerto del FileSystem es: %d \n",config->PUERTO_FILESYSTEM);
	}

	if (config_has_property(configNodo, "IP_NODO")){
		config->IP_NODO= config_get_string_value(configNodo,"IP_NODO");
		printf("La IP del Nodo es %s \n",config->IP_NODO);
	}

	if (config_has_property(configNodo, "PUERTO_WORKER")){
		config->PUERTO_WORKER = config_get_int_value(configNodo,"PUERTO_WORKER");
		printf("El puerto del Worker es: %d \n",config->PUERTO_WORKER);
	}

	if (config_has_property(configNodo, "PUERTO_DATANODE")){
		config->PUERTO_DATANODE = config_get_int_value(configNodo,"PUERTO_DATANODE");
		printf("El puerto del DataNote es: %d \n",config->PUERTO_DATANODE);
	}

	if (config_has_property(configNodo, "NOMBRE_NODO")){
		config->NOMBRE_NODO = config_get_string_value(configNodo,"NOMBRE_NODO");
		printf("El nombre del Nodo es %s \n",config->NOMBRE_NODO);
	}

	if (config_has_property(configNodo, "RUTA_DATABIN")){
		config->RUTA_DATABIN = config_get_string_value(configNodo,"RUTA_DATABIN");
		printf("La ruta del DataBin es %s \n",config->RUTA_DATABIN);
	}

	if (config_has_property(configNodo, "SIZE_NODO")){
		config->SIZE_NODO = config_get_int_value(configNodo,"SIZE_NODO");
		printf("El tamaño del DataNote es: %d \n",config->SIZE_NODO);
	}
}

void cargarConfiguracionFileSystem(struct configuracionFileSystem *config,char* rutaAConfig){
	char* pathArchConfig = obtenerRutaRealConfiguracion(rutaAConfig);

	t_config* configFS = config_create(pathArchConfig);

	if (config_has_property(configFS, "IP_FILESYSTEM")){
		config->IP_FILESYSTEM= config_get_string_value(configFS,"IP_FILESYSTEM");
		printf("La IP del FileSystem es %s \n",config->IP_FILESYSTEM);
	}

	if (config_has_property(configFS, "PUERTO_FILESYSTEM")){
		config->PUERTO_FS= config_get_int_value(configFS,"PUERTO_FILESYSTEM");
		printf("El puerto del FileSystem es: %d \n",config->PUERTO_FS);
	}
}

char* obtenerRutaRealConfiguracion(char* rutaAConfig){
	char cwd [1024];    // Variable donde voy a guardar el path absoluto hasta el /Debug
	char* pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)), rutaAConfig); // String que va a tener el path absoluto para pasarle al config_create
	return pathArchConfig;
}
