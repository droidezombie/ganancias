/*
 * FuncionesFS.h
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <commons/log.h>
#include "Globales.h"
#include <commons/collections/list.h>
#include <semaphore.h>
#include <pthread.h>
#include "Comandos.h"
#include <math.h>
#include <sys/types.h>

#define idDataNodes 3
#define idWorker 100
#define cantDataNodes 10
#define mb 1048576

extern int numeroCopiasBloque;
t_directory tablaDeDirectorios[100];
extern char* rutaArchivos;
extern t_log* loggerFS;
extern int cantidadDirectorios;
extern int cantBloques;
extern int sizeTotalNodos, nodosLibres;
extern t_list* bitmapsNodos;;
extern t_list* nodosConectados;
extern char* rutaBitmaps;
pthread_mutex_t logger_mutex;
extern int EstadoFS;
extern bool fsFormateado;

void verificarEstadoAnterior();

char* nombreArchivoSinExtension(char* nombre);

char* rutaArchivoMetadataSinExtension(char* ruta);

int validarArchivoYamaFS(char* ruta);

void inicializarTablaDirectorios();

char* buscarNombreEnMetadata(char* ruta);

char* rutaSinPrefijoYama(char* ruta);

int verificarEstado();

int bytesACortar(char* mapa, int offset, int sizeRestante);

void* leerDeDataNode(void* parametros);

void guardarTablaDirectorios();

char* buscarRutaArchivo(char* ruta);

int getIndexDirectorio(char* ruta);

char* generarArrayNodos();

int levantarBitmapNodo(int numeroNodo, int sizeNodo);

int buscarPrimerBloqueLibre(int numeroNodo, int sizeNodo);

void actualizarArchivoNodos();

int nodoRepetido(informacionNodo info);

void atenderSolicitudYama(int socketYama, void* envio);

char* generarArrayBloque(int numeroNodo, int numeroBloque);

int guardarEnNodos(char* path, char* nombre, char* tipo, string* mapeoArchivo);

void obtenerLinea(char* resultado, char* archivo, int offset);

void setearBloqueOcupadoEnBitmap(int numeroNodo, int bloqueLibre);

void setearBloqueLibreEnBitmap(int numeroNodo, int bloqueOcupado);

bool esBloqueOcupado(int numeroNodo, int numeroBloque);

int* arrayBloquesOcupados(informacionNodo nodo);

void actualizarBitmapNodos();

void* enviarADataNode(void* parametros);

informacionNodo* informacionNodosConectados();

void establecerServidor(char* ip, int port);

int recibirConexionYama();

informacionArchivoFsYama obtenerInfoArchivo(string rutaDatos);

void obtenerInfoNodo(ubicacionBloque* ubicacion);

char* leerArchivo(char* rutaArchivo);

char* ultimaParteDeRuta(char* rutaArchivo);

int guardarBloqueEnNodo(int bloque, int nodo, t_config* infoArchivo);

int obtenerNumeroCopia(t_config* infoArchivo,int bloqueACopiar);

int borrarDirectorios();

int borrarArchivosEnMetadata();

int liberarNodosConectados();

int formatearDataBins();

char* devolverRuta(char* comando, int posicionPalabra);

bool isDirectoryEmpty(char *dirname);

int esRutaDeYama(char* ruta);

int borrarDeArchivoMetadata(char* ruta, int bloque, int copia);

int longitudAntesDelIgual(char* cadena);
