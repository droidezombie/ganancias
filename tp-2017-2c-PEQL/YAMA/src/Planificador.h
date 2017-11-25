/*
 * Planificador.h
 *
 *  Created on: 3/10/2017
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "FuncionesYama.h"
#include "../BibliotecasCompartidas/Globales.h"
#include <pthread.h>
#include <semaphore.h>
#include <../commons/string.h>

t_list* jobsAPlanificar;

pthread_mutex_t cantTareasHistoricas_mutex;
uint32_t wlMax;

void planificar(job* job);
infoNodo* inicializarWorker();
void verificarDisponibilidades(t_list* listaNodos);
void iniciarListasPlanificacion();
void seleccionarWorker(infoNodo* worker, infoBloque bloque);
bool mayorDisponibilidad(infoNodo* worker, infoNodo* workerMasDisp);
infoNodo* buscarNodo(t_list* nodos, int numNodo);
uint32_t calcularPWL(infoNodo* worker);
uint32_t workLoadMaxima();
void calcularWorkLoadMaxima(t_list* nodos);
void calcularDisponibilidadWorkers(t_list* nodos);
void calcularDisponibilidadWorker(infoNodo* worker);
int obtenerDisponibilidadWorker(infoNodo* worker);
void agregarNodo(t_list* cargaNodo,infoNodo* nodo);
void agregarJobAPlanificar(job* jobAPlanificar);
uint32_t cargaMaxima();
void agregarNodos(t_list* cargaNodos, t_list* listaNodos);
void iniciarListasPlanificacion();
t_list* consultarDetallesBloqueArchivo(char *pathArchivo, int bloque);
void calcularCargasDeWorkers(t_list* listaNodos);
void bloqueEstaEnWorker(infoBloque* bloque, infoNodo* worker);
informacionArchivoFsYama* recibirInfoArchivo(job* job) ;
bool estaActivo(infoNodo* worker);
infoNodo* posicionarClock(t_list* listaWorkersConBloques);
bool bloqueEstaEn(infoNodo* nodo,bool** nodoXbloque, int bloque);
respuestaSolicitudTransformacion* moverClock(infoNodo* workerDesignado, t_list* listaNodos, bool** nodosPorBloque, t_list* infoArchivo,int job);
infoNodo* avanzarClock(infoNodo* worker, t_list* listaNodos);
void modificarCargayDisponibilidad(infoNodo* worker);
void restaurarDisponibilidad(infoNodo* worker);
infoNodo* obtenerProximoWorkerConBloque(t_list* listaNodos,int bloque,int numWorkerActual);
void agregarBloqueANodoParaEnviar(infoBloque* bloque,infoNodo* nodo,respuestaSolicitudTransformacion* respuestaMaster,int job);
void verificarValorDisponibilidad(infoNodo* nodo);
void agregarInfoTransformacionATablaDeEstadoos(informacionArchivoFsYama* infoArchivo,int jobid);
void enviarReduccionLocalAMaster(job* job,int nodo);
void enviarReduccionGlobalAMaster(job* job);
void replanificar(int paraReplanificar,job* jobi,respuestaSolicitudTransformacion* respuestaArchivo, bool** matrix,int bloques,int nodos);
int calcularNodoEncargado(t_list* registrosRedGlobal);
t_list* buscarCopiasBloques(t_list* listaBloques,t_list* listaNodos,informacionArchivoFsYama* infoArchivo);
void realizarAlmacenamientoFinal(job* job);
void planificarReduccionesLocales(job* job,bool** matrix,respuestaSolicitudTransformacion* respuestaMaster,int nodos,int bloques);

#endif /* PLANIFICADOR_H_ */
