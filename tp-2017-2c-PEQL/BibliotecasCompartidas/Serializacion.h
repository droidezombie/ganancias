/*
 * Serializacion.h
 *
 *  Created on: 7/9/2017
 *      Author: utnso
 */

#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Globales.h"
#include <commons/collections/list.h>

#define mensajeDesconexion -1
#define mensajeHandshake 1
#define mensajeArchivo 2
#define mensajeOk 3
#define mensajeSolicitudTransformacion 4
#define mensajeEtapaTransformacion 5
#define mensajeEtapaReduccionLocal 6
#define mensajeEtapaReduccionGlobal 7
#define mensajeInformacionNodo 8
#define mensajeProcesarTransformacion 10
#define mensajeProcesarRedLocal 11
#define mensajeDesignarEncargado 12
#define mensajeDesignarWorker 14
#define mensajeInfoArchivo 15
#define mensajeEnvioBloqueANodo 16
#define mensajeRespuestaEnvioBloqueANodo 17
#define mensajeNumeroCopiaBloqueANodo 18
#define mensajeRespuestaGetBloque 19
#define mensajeSolicitudInfoNodos 20
#define mensajeRespuestaInfoNodos 21
#define mensajeNumeroLecturaBloqueANodo 22
#define mensajeRespuestaTransformacion 23
#define mensajeError 24
#define mensajeSizeLecturaBloqueANodo 25
#define mensajeTransformacionCompleta 26
#define mensajeFalloTransformacion 27
#define mensajeRedLocalCompleta 28
#define mensajeFalloRedLocal 29
#define mensajeRedGlobalCompleta 30
#define mensajeFalloRedGlobal 31
#define mensajeFinJob 32
#define mensajeRespuestaRedLocal 33
#define mensajeRespuestaRedGlobal 34
#define mensajeReplanificacion 35
#define mensajeFalloReduccion 36
#define mensajeBorraDataBin 37
#define mensajeRespuestaBorraDataBin 38
#define mensajeSolicitudArchivo 39
#define mensajeProcesarRedGlobal 40
#define mensajeNoEstable 41
#define mensajeRespuestaSolicitudArchivo 42
#define mensajeRespuestaAlmacenamiento 43
#define mensajeAlmacenamientoCompleto 44
#define mensajeFalloAlmacenamiento 45
#define mensajeProcesarAlmacenamiento 46
#define mensajeAlmacenar 47
#define mensajeConectado 48

typedef struct{
	int idMensaje;
	int tamanio;
} header;

typedef struct{
	int idMensaje;
	void* envio;
	int size;
} respuesta;

void empaquetar(int socket, int idMensaje,int tamanioS, void* paquete);
respuesta desempaquetar(int socket);

void* serializarString(void* paquete,int *tamanio);
string* deserializarString(int socket,int tamanio);

void* serializarJob(void* paquete, int* tamanio);
job* deserializarJob(int socket, int tamanio);

void* serializarSolicitudInfoNodos(void* paquete,int* tamanio);
solicitudInfoNodos* deserializarSolicitudInfoNodos(int socket,int tamanio);

void* serializarInformacionNodos(void* paquete,int* tamanio);
informacionNodo* deserializarInformacionNodos(int socket,int tamanio);

void* serializarRespuestaInfoNodos(void* paquete,int* tamanio);
informacionArchivoFsYama* deserializarRespuestaInfoNodos(int socket,int tamanio);

void* serializarRespuestaTransformacion(void* paquete,int* tamanio);
respuestaSolicitudTransformacion* deserializarRespuestaTransformacion(int socket,int tamanio);

void* serializarRespuestaRedLocal(void* paquete,int* tamanio);
nodosRedLocal* deserializarRespuestaRedLocal(int socket,int tamanio);

void* serializarRespuestaRedGlobal(void* paquete,int* tamanio);
respuestaReduccionGlobal* deserializarRespuestaRedGlobal(int socket,int tamanio);

void* serializarProcesarTransformacion(void* paquete, int* tamanio);
parametrosTransformacion* deserializarProcesarTransformacion(int socket, int tamanio);

void* serializarBloqueYNodo(void* paquete, int* tamanio);
bloqueYNodo* deserializarBloqueYNodo(int socket, int tamanio);

void* serializarReplanificacion(void* paquete, int* tamanio);
workerDesdeYama* deserializarReplanificacion(int socket, int tamanio);

void* serializarProcesarRedLocal(void* paquete, int* tamanio);
parametrosReduccionLocal* deserializarProcesarRedLocal(int socket, int tamanio);

void* serializarProcesarRedGlobal(void* paquete, int* tamanio);
parametrosReduccionGlobal* deserializarProcesarRedGlobal(int socket, int tamanio);

void* serializarSolicitudArchivo(void* paquete, int* tamanio);
string* deserializarSolicitudArchivo(int socket, int tamanio);

void* serializarRespuestaAlmacenamiento(void* paquete, int* tamanio);
respuestaAlmacenamiento* deserializarRespuestaAlmacenamiento(int socket, int tamanio);

void* serializarProcesarAlmacenamiento(void* paquete, int* tamanio);
parametrosAlmacenamiento* deserializarProcesarAlmacenamiento(int socket, int tamanio);

void* serializarAlmacenar(void* paquete, int* tamanio);
almacenamientoFinal* deserializarAlmacenar(int socket, int tamanio);

#endif /* SERIALIZACION_H_ */
