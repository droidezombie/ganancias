/*
 * Globales.h
 *
 *  Created on: 7/9/2017
 *      Author: utnso
 */

#ifndef GLOBALES_H_
#define GLOBALES_H_

#include <stdbool.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <stdint.h>

sem_t pedidoFS;
sem_t actualizarNodos;

typedef struct string{
	int longitud;
	char* cadena;
}string;

typedef struct job{
	int id;
	int socketFd;
	string rutaTransformador;
	string rutaReductor;
	string rutaDatos;
	string rutaResultado;
}job;

typedef enum {TRANSFORMACION, RED_LOCAL, RED_GLOBAL, ALM_FINAL}Etapa;

typedef enum {EN_EJECUCION, ERROR, FINALIZADO_OK}Estado;

typedef enum {OK,FALLO_ALMACENAMIENTO,FALLO_RED_GLOBAL,FALLO_RED_LOCAL,FALLO_TRANSFORMACION,FALLO_INGRESO}Finalizacion;


typedef struct {
	t_list* workers;
}respuestaSolicitudTransformacion;

typedef struct {
	int numeroWorker;
	string ip;
	int puerto;
	t_list* bloquesConSusArchivos;
}workerDesdeYama;

typedef struct{
	int numeroNodo;
	int puerto;
	string ip;
	string archivoTemporal;
	t_list* archivos;
}nodosRedLocal;

typedef struct{
	//infoBloque bloque;
	int numBloque;
	int numBloqueEnNodo;
	int bytesOcupados;
	string archivoTemporal;
}bloquesConSusArchivosTransformacion;

typedef struct {
	int nodo;
	char* ip;
	int puerto;
	int bloque;
	int byteOcupado;
	char* rutaArchivoTemp;
}respuestaInfoNodos;

typedef struct {
	int numBloque;
	int numBloqueEnNodo;
	string archivoTransformacion;
	string archivoReduccion;
}bloquesConSusArchivosRedLocal;

typedef struct {
	t_list* workers;
}respuestaReduccionLocal;

typedef struct respuestaReduccionGlobal{
	int puerto;
	string ip;
	string archivoReduccionLocal;
	bool encargado;
}bloquesConSusArchivosRedGlobal;

typedef struct{
	int puerto;
	string ip;
	int nodo;
	string archivo;
}respuestaAlmacenamiento;

typedef struct{
	string rutaAlmacenamiento;
	string archivoTemporal;
}parametrosAlmacenamiento;

typedef struct {
	int puerto;
	string ip;
	string nombreArchivoReducido;
}infoWorker;

typedef struct{
	t_list* infoWorkers;
	string contenidoScript;
	string archivoTemporal;
}parametrosReduccionGlobal;

typedef struct {
	parametrosReduccionGlobal* parametros;
	int puerto;
	string ip;
	string archivoTemporal;
	int numero;
	int job;
}respuestaReduccionGlobal;

typedef struct registroTablaEstados{
	int job;
	int master;
	int nodo;
	int bloque;
	Etapa etapa;
	char* rutaArchivoTemp;
	Estado estado;
}registroTablaEstados;

typedef struct {
	string rutaDatos;
	string rutaResultado;
}solicitudInfoNodos;

typedef struct{
	int sizeNodo;
	int bloquesOcupados;
	int numeroNodo;
	int socket;
	int puerto;
	string ip;
} informacionNodo;

typedef struct{
	string ip;
	int puerto;
	int numeroBloqueEnNodo;
	int numeroNodo;
}ubicacionBloque;

typedef struct {
	int bytesOcupados;
	int numeroBloque;
	ubicacionBloque ubicacionCopia0;
	ubicacionBloque ubicacionCopia1;
}infoBloque;

typedef struct{
	int tamanioTotal;
	t_list* informacionBloques;
}informacionArchivoFsYama;

typedef struct{
	int index;
	char nombre[255];
	int padre;
} t_directory;

typedef struct parametrosEnvioBloque{
    int socket;
    char* mapa;
    int sizeBloque;
    int offset;
    int bloque;
    int restanteAnterior;
    int sem;
}parametrosEnvioBloque;

typedef struct parametrosLecturaBloque{
    int socket;
    int bloque;
    char* contenidoBloque;
    int sem;
    int sizeBloque;
}parametrosLecturaBloque;

typedef struct {
	int cantTareas[3];
	int cantFallos;
	time_t tiempoInicio;
	t_list* tiempoInicioTrans;
	t_list* tiempoInicioRedGlobal;
	t_list* tiempoInicioRedLocal;
	t_list* tiempoFinTrans;
	t_list* tiempoFinRedGlobal;
	t_list* tiempoFinRedLocal;
}estadisticaProceso;

typedef struct{
	bool finalizo;
	time_t tiempo;
	int numero;
}tiempoPorBloque;

typedef struct {
	int numero;
	string ip;
	int puerto;
	uint32_t carga;
	t_list* bloques;
	bool conectado;
	int cantTareasHistoricas;
	int disponibilidad;
}infoNodo;

typedef struct{
	int numero;
	int puerto;
	string ip;
	bloquesConSusArchivosTransformacion bloquesConSusArchivos;
	string contenidoScript;
} parametrosTransformacion;

typedef struct{
	int numero;
	int puerto;
	string ip;
	t_list* archivosTemporales;
	string rutaDestino;
	string contenidoScript;
} parametrosReduccionLocal;

typedef struct{
	int bloque;
	int workerId;
}bloqueYNodo;

typedef struct{
	string nombre;
	string contenido;
}almacenamientoFinal;

int redondearHaciaArriba(int num,int num2);

bool validarArchivo(char* path);

bool validarDirectorio(char* path);

char* rutaSinArchivo(char* rutaArchivo);

void obtenerNumeroNodo(t_config* archivo,char* claveCopia,ubicacionBloque* ubi);

void limpiarPantalla();

#endif /* GLOBALES_H_ */
