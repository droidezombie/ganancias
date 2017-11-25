/*
 * FuncionesHilos.h
 *
 *  Created on: 11/10/2017
 *      Author: utnso
 */

#include "FuncionesFS.h"

void* levantarServidorFS();

void* consolaFS();

void* manejarConexionYama();

int nodoDeEstadoAnterior(informacionNodo info);

int validarParametros(char** arguments, int cantidadParametros);

void revisarNodos();
