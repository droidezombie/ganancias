/*
 * Sockets.h
 *
 *  Created on: 18/4/2017
 *      Author: utnso
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include "Serializacion.h"
#include <netinet/in.h>
#include <sys/select.h>

struct t_tam{
	int menu;
	int length;
};

int crearSocket();

bool conectarCon(struct sockaddr_in direccionServidor, int cliente, int tipoCliente);

void enviarMensajeA(int *socket, int longitud);

struct sockaddr_in cargarDireccion(char* direccionIP, int puerto);

void recibirMensajes(int cliente);

void asociarSocketA(struct sockaddr_in direccionServidor, int servidor);

typedef struct parametrosServidorHilo{
	int servidor;
	int cliente;
}parametrosServidorHilo;

void levantarServidorYama(char* ip, int port);

int crearServidorAsociado(char* ip, int puerto);

#endif /* SOCKETS_H_ */
