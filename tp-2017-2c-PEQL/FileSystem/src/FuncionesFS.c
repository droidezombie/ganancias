/*
 * FuncionesFS.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <Configuracion.h>
#include <sys/types.h>
#include <commons/bitarray.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "FuncionesFS.h"
#include "Comandos.h"
#include "Serializacion.h"
#include <dirent.h>
#include <errno.h>
#include "Serial.h"
#include <Globales.h>
#include <math.h>

char* pathArchivoDirectorios = "../metadata/Directorios.dat";
char* pathArchivoNodos = "../metadata/Nodos.bin";
char* rutaMetadataBitmaps = "../metadata/Bitmaps";
struct sockaddr_in direccionCliente;
unsigned int tamanioDireccion = sizeof(direccionCliente);
int servidorFS;
extern t_list* pedidosFS;
int sizeBloque;
int successArchivoCopiado;
extern int bloquesLibresTotales;
sem_t sem;
extern sem_t pedidoTerminado;
extern pthread_mutex_t listSemMutex;
int bla = 0;
extern bool recuperarEstado;


void establecerServidor(char* ip, int port){
	struct sockaddr_in direccionServidor = cargarDireccion(ip, port);
	int activado = 1;
	setsockopt(servidorFS, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	asociarSocketA(direccionServidor, servidorFS);
}

void verificarEstadoAnterior(){
	recuperarEstado = validarArchivo(pathArchivoNodos);
	printf("rec %d\n", recuperarEstado);
}
int verificarEstado(){
    DIR* directorio;
	struct dirent* in_file;
	char* path;
	char* pathDirTemplate = "../metadata/Archivos/";
	char* pathDir;
	int i = 0, j = 0, l = 0, k = 0;
	int cantidadNodosConectados = list_size(nodosConectados);
	int nodos[cantidadNodosConectados];
	int sizeArchivo, cantBloques = 1;
	informacionNodo info;
	t_config* infoArchivo;
	char** arrayInfoBloque;
	int numeroNodo;
	char* charNumeroNodo;
	char* bloque;
	int valido = 0;

	for (i = 0; i < cantidadNodosConectados; ++i){
		info = *(informacionNodo*)list_get(nodosConectados,i);
		nodos[i] = info.numeroNodo;
	}

	for (i = 0; i < 100; ++i){

		if(tablaDeDirectorios[i].index == -1)
			continue;

		pathDir = string_from_format("%s%d",pathDirTemplate,i);
		printf("%s dir \n", pathDir);
		if (NULL == (directorio = opendir (pathDir)))
			{
				continue;
			}
			while ((in_file = readdir(directorio)))
			{
				if (!strcmp (in_file->d_name, "."))
					continue;
				if (!strcmp (in_file->d_name, ".."))
					continue;

				path = string_from_format("%s/%s",pathDir,in_file->d_name);
				printf("%s \n", path);
				infoArchivo = config_create(path);

				if (config_has_property(infoArchivo, "TAMANIO")){
					sizeArchivo = config_get_int_value(infoArchivo,"TAMANIO");
				}

				if (sizeArchivo > mb)
					cantBloques = redondearHaciaArriba(sizeArchivo, mb);
				else
					cantBloques = 1;

				for (j = 0; j < cantBloques; ++j){
					for (l = 0; l < numeroCopiasBloque; ++l){
						bloque = string_from_format("BLOQUE%dCOPIA%d",j,l);
						if (config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",j,l))){
							arrayInfoBloque = config_get_array_value(infoArchivo, bloque);
							charNumeroNodo = string_substring_from(arrayInfoBloque[0], 4);
							numeroNodo = atoi(charNumeroNodo);

							for (k = 0; k < cantidadNodosConectados; ++k)
								if (nodos[k] == numeroNodo){
									++valido;
								}
						}

					}
					if (valido < numeroCopiasBloque){
						free(bloque);
						free(charNumeroNodo);
						free(path);
						free(pathDir);
						return 0;
					}

					valido = 0;

					free(bloque);
					free(charNumeroNodo);
				}
				free(path);
				config_destroy(infoArchivo);
			}
			free(pathDir);
		}
	//actualizarArchivoNodos();
	return 1;
}

char* buscarNombreEnMetadata(char* ruta){
	t_config* config = config_create(ruta);
	char* nombre;
	char* path;
	int indexNombre = 0;
	if (config_has_property(config,"RUTA")){
		path = config_get_string_value(config, "RUTA");
		path = string_substring_from(path,7);
		char* reverse = string_reverse(path);
		char* currentChar = string_substring(path,indexNombre,1);
		while (strcmp(currentChar,"/")){
			++indexNombre;
			free(currentChar);
			currentChar = string_substring(path,indexNombre,1);
		}
		nombre = string_substring_until(path,indexNombre);
		free(reverse);
		free(currentChar);
		printf("nombre %s", string_reverse(nombre));
		config_destroy(config);
		return string_reverse(nombre);

	}
	else
		return "-1";

}

void inicializarTablaDirectorios(){
	int i;
	struct stat fileStat;
	FILE* archivoDirectorios;
	char* root = "../metadata/Archivos/0";

	tablaDeDirectorios[0].index = 0;
	tablaDeDirectorios[0].padre = -1;
	memcpy(tablaDeDirectorios[0].nombre,"/",1);

	if (!validarDirectorio(root))
		mkdir(root,0777);

	for (i = 1; i < 100; ++i){
		tablaDeDirectorios[i].index = -1;
		tablaDeDirectorios[i].padre = -1;
		memcpy(tablaDeDirectorios[i].nombre," ",1);
	}

	archivoDirectorios = fopen(pathArchivoDirectorios, "ab");
	fclose(archivoDirectorios);

	int leido;
	if(stat(pathArchivoDirectorios,&fileStat) < 0){
		printf("no se pudo abrir\n");
		exit(1);
	}

	if (fileStat.st_size != 0){
		archivoDirectorios = fopen(pathArchivoDirectorios, "r");
		leido = fread(tablaDeDirectorios,sizeof(t_directory), cantidadDirectorios, archivoDirectorios);
		fclose(archivoDirectorios);
	}
	else
		leido = 0;

	if (leido == cantidadDirectorios)
		log_trace(loggerFS, "Se cargaron los directorios correctamente");
	else
		log_trace(loggerFS, "No se pudieron cargar todos los directorios");
}

void guardarTablaDirectorios(){
	int escrito;
	FILE* archivoDirectorios = fopen(pathArchivoDirectorios, "w");
	escrito = fwrite(tablaDeDirectorios,sizeof(t_directory), cantidadDirectorios, archivoDirectorios);
	if (escrito == cantidadDirectorios)
		log_trace(loggerFS, "Se escribieron los directorios correctamente");
	else
		log_trace(loggerFS, "No se pudieron escribir todos los directorios");
	fclose(archivoDirectorios);
}

int getIndexDirectorio(char* ruta){
	int index = 0, i = 0, j = 0, indexFinal = 0;

	if(strcmp(ruta, "/") == 0) //si la ruta es root devuelvo 0
		return 0;

	char* rutaSinRoot = string_substring_from(ruta,1);
	char** arrayPath = string_split(rutaSinRoot, "/"); //le saco root a la ruta
	char* arrayComparador[100];
	while(arrayPath[index] != NULL){ // separo por '/' la ruta en un array
		arrayComparador[index] = malloc(strlen(arrayPath[index])+1);
		memset(arrayComparador[index],0 ,strlen(arrayPath[index])+1);
		memcpy(arrayComparador[index],arrayPath[index],strlen(arrayPath[index]));
		++index; // guardo cuantas partes tiene el array
	}
	indexFinal = index;
	int indexDirectorios[index];

	for (i = 0; i <= index; ++i)
		indexDirectorios[i] = -1; // inicializo todo en -1, si al final me queda alguno en algun lado del array, significa que la ruta que pase no existe
	--index;

	indexDirectorios[0] = 0;

	for(i = 0; i <= index; --index){
		for(j = 0; j < 100; ++j){ // busco en la tabla que posicion coincide con el fragmento de ruta del loop
			if (strcmp(tablaDeDirectorios[j].nombre,arrayComparador[index]) == 0){ // me fijo si el padre de ese es el mismo que aparece en el fragmento anterior de la ruta
				if ((index != 0 && strcmp(tablaDeDirectorios[tablaDeDirectorios[j].padre].nombre,arrayComparador[index-1]) == 0) || (index == 0 && tablaDeDirectorios[j].padre == 0)){ // si es asi lo guardo
					indexDirectorios[index+1] = tablaDeDirectorios[j].index;
				}
			}
		}
	}

	for (i = 0; i < indexFinal; ++i)
		if (indexDirectorios[i] == -1){
			for (j = 0; j < indexFinal; ++j){
				free(arrayComparador[j]);
			}
			return -1;
		} //me fijo si quedo algun -1, si es asi no existe la ruta

	for (i = 0; i < indexFinal; ++i){
		free(arrayComparador[i]);
	}
	return indexDirectorios[indexFinal]; // devuelvo el index de la ruta
}

char* buscarRutaArchivo(char* ruta){
	ruta = rutaSinPrefijoYama(ruta);
	//printf("-->%s\n", ruta);
	int indexDirectorio = getIndexDirectorio(ruta);
	//printf("-->%d\n", indexDirectorio);
	if (indexDirectorio == -1)
		return "-1";
	//printf("%s\n", ruta);
	char* numeroIndexString = string_itoa(indexDirectorio);
	char* rutaGenerada = calloc(1,strlen(rutaArchivos) + strlen(numeroIndexString) + 1);
	memset(rutaGenerada,0,strlen(rutaArchivos) + strlen(numeroIndexString) + 1);
	memcpy(rutaGenerada, rutaArchivos, strlen(rutaArchivos));
	memcpy(rutaGenerada + strlen(rutaArchivos), numeroIndexString, strlen(numeroIndexString));
	return rutaGenerada; //poner free despues de usar
}

int nodoRepetido(informacionNodo info){
	int cantidadNodos = list_size(nodosConectados);
	int i = 0, repetido = 0;
	informacionNodo infoAux;
	for (i = 0; i < cantidadNodos; ++i){
		infoAux = *(informacionNodo*)list_get(nodosConectados,i);
		if (infoAux.numeroNodo == info.numeroNodo)
			repetido = 1;
	}
	return repetido;
}

int validarArchivoYamaFS(char* ruta){
	if (string_starts_with(ruta,"yamafs:/")){
		//printf("ruta valida\n");
		return 1;
	}
	//printf("ruta invalida\n");
	return 0;
}

char* rutaSinPrefijoYama(char* ruta){
	if (validarArchivoYamaFS(ruta) == 0)
		return ruta;
	else
		return string_substring_from(ruta, 7);
}



char* ultimaParteDeRuta(char* rutaArchivo){
	int index = 0;
	char* rutaInvertida = string_reverse(rutaArchivo);
	char* currentChar = malloc(2);
	char* nombreInvertido = malloc(strlen(rutaArchivo)+1);

	memset(nombreInvertido,0,strlen(rutaArchivo)+1);
	memset(currentChar,0,2);

	currentChar = string_substring(rutaInvertida, index, 1);
	while(strcmp(currentChar,"/")){
		memcpy(nombreInvertido + index, currentChar, 1);
		++index;
		currentChar = string_substring(rutaInvertida, index, 1);
	}

	free(currentChar);

	char* nombre = string_reverse(nombreInvertido);
	free(nombreInvertido);

	return nombre;
}

char* rutaArchivoMetadataSinExtension(char* ruta){
	char* path = buscarRutaArchivo(rutaSinArchivo(rutaSinPrefijoYama(ruta)));
	return string_from_format("%s/%s", path, nombreArchivoSinExtension(ultimaParteDeRuta(ruta)));
}

char* leerArchivo(char* rutaArchivo){
	int index = 0, sizeArchivo, cantBloquesArchivo = 0, sizeAux = 0, i, j, k, l;
	int cantidadNodos = list_size(nodosConectados);
	int cargaNodos[cantidadNodos], indexNodos[cantidadNodos], peticiones[cantidadNodos];
	int numeroNodoDelBloque[numeroCopiasBloque];
	informacionNodo info;
	char* rutaInvertida = string_reverse(rutaArchivo);
	char* rutaFinal = malloc(strlen(rutaArchivo)+1);
	char* currentChar = malloc(2);
	char* nombreInvertido = malloc(strlen(rutaArchivo)+1);
	char** arrayInfoBloque;
	int posicionNodoAPedir = -1;
	int numeroBloqueDataBin = -1;

	int copiaUsada;

	//sem_init(&sem,1,0);

	memset(nombreInvertido,0,strlen(rutaArchivo)+1);
	memset(rutaFinal,0,strlen(rutaArchivo)+1);
	memset(currentChar,0,2);

	currentChar = string_substring(rutaInvertida, index, 1);
	while(strcmp(currentChar,"/")){
		memcpy(nombreInvertido + index, currentChar, 1);
		++index;
		currentChar = string_substring(rutaInvertida, index, 1);
	}


	char* nombre = malloc(index + 1);
	memset(nombre,0,index + 1);
	memcpy(nombre, string_reverse(nombreInvertido), index);

	memcpy(rutaFinal, rutaArchivo, strlen(rutaArchivo)-index);

	char* rutaMetadata = buscarRutaArchivo(rutaFinal);
	char* rutaArchivoEnMetadata = malloc(strlen(rutaMetadata) + strlen(nombre) + 2);
	memset(rutaArchivoEnMetadata,0,strlen(rutaMetadata) + strlen(nombre) + 2);
	memcpy(rutaArchivoEnMetadata, rutaMetadata, strlen(rutaMetadata));
	memcpy(rutaArchivoEnMetadata + strlen(rutaMetadata), "/", 1);
	memcpy(rutaArchivoEnMetadata + strlen(rutaMetadata) + 1, nombre, strlen(nombre));

	t_config* infoArchivo = config_create(rutaArchivoEnMetadata);
	FILE* informacionArchivo = fopen(rutaArchivoEnMetadata,"r");

	if (config_has_property(infoArchivo, "TAMANIO")){
		sizeArchivo = config_get_int_value(infoArchivo,"TAMANIO");
	}

	char* lectura = malloc(sizeArchivo + 1);
	memset(lectura, 0, sizeArchivo + 1);

	sizeAux = sizeArchivo;

	while(config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA0",cantBloquesArchivo))){
		sizeAux -= mb;
		++cantBloquesArchivo;
	}

	int cantidadCopiasBloque[cantBloquesArchivo];

	for (j = 0; j < cantBloquesArchivo; ++j)
		cantidadCopiasBloque[j] = 0;

	int maxCopias = 0;

	for (j = 0; j < cantBloquesArchivo; ++j){
		for (i = 0; config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",j,i)); ++i){
			++cantidadCopiasBloque[j];
		}
		if (cantidadCopiasBloque[j] > maxCopias)
			maxCopias = cantidadCopiasBloque[j];
	}

	int posicionCopiaEnIndexNodo[maxCopias];
	int copia[maxCopias];

	char* respuestas[cantBloquesArchivo];
	parametrosLecturaBloque params[cantBloquesArchivo];
	pthread_t nuevoHilo[cantBloquesArchivo];

	for (j = 0; j < cantidadNodos; ++j){
		info = *(informacionNodo*)list_get(nodosConectados, j);
		indexNodos[j] = info.numeroNodo;
		cargaNodos[j] = 0;
		peticiones[j] = 0;
	}

	for (i = 0; i < cantBloquesArchivo; ++i){
		for (l = 0; l < cantidadCopiasBloque[i]; ++l){
			arrayInfoBloque = config_get_array_value(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",i,l));
			numeroNodoDelBloque[l] = atoi(string_substring_from(arrayInfoBloque[0], 4));
			for (k = 0; k < cantidadNodos; ++k)
				if (indexNodos[k] == numeroNodoDelBloque[l])
					++cargaNodos[k];
			free(arrayInfoBloque);
		}
	}

	for (i = 0; i < cantBloquesArchivo; ++i){
		for (l = 0; l < cantidadCopiasBloque[i]; ++l){
			arrayInfoBloque = config_get_array_value(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",i,l));
			copia[l] = atoi(arrayInfoBloque[1]);
			numeroNodoDelBloque[l] = atoi(string_substring_from(arrayInfoBloque[0], 4));
			for (k = 0; k < cantidadNodos; ++k)
				if (indexNodos[k] == numeroNodoDelBloque[l]){
					posicionCopiaEnIndexNodo[l] = k;
				}
			free(arrayInfoBloque);
		}
		posicionNodoAPedir = posicionCopiaEnIndexNodo[0];
		copiaUsada = copia[0];
		for (l = 0; l < cantidadCopiasBloque[i]; ++l){
			if (peticiones[posicionCopiaEnIndexNodo[l]] < peticiones[posicionNodoAPedir]){
				posicionNodoAPedir = posicionCopiaEnIndexNodo[l];
				copiaUsada = copia[l];
			}
			else if (peticiones[posicionCopiaEnIndexNodo[l]] == peticiones[posicionNodoAPedir])
				if(cargaNodos[posicionCopiaEnIndexNodo[l]] < cargaNodos[posicionNodoAPedir]){
					posicionNodoAPedir = posicionCopiaEnIndexNodo[l];
					copiaUsada = copia[l];
				}
		}

		info = *(informacionNodo*)list_get(nodosConectados,posicionNodoAPedir);
		++peticiones[posicionNodoAPedir];
		numeroBloqueDataBin = copiaUsada;

		if (config_has_property(infoArchivo, string_from_format("BLOQUE%dBYTES",i))){
			params[i].sizeBloque = config_get_int_value(infoArchivo,string_from_format("BLOQUE%dBYTES",i));
			//printf("size %d \n",params[i].sizeBloque);
		}


		params[i].bloque = numeroBloqueDataBin;
		params[i].socket = info.socket;
		params[i].sem = posicionNodoAPedir;

		pthread_create(&nuevoHilo[i], NULL, &leerDeDataNode,(void*) &params[i]);
		//sem_wait(&pedidoFS);

		//printf("size bloque yay %d\n", strlen(respuestas[i]));


	}
	int contador = 0;
	for (i = 0; i < cantBloquesArchivo; ++i){
		pthread_join(nuevoHilo[i], (void**)&respuestas[i]);
	}
	for (i = 0; i < cantBloquesArchivo; ++i){
		memcpy(lectura + contador, respuestas[i], strlen(respuestas[i]));
		contador += strlen(respuestas[i]);
		//printf("%s\n", respuestas[i]);
		free(respuestas[i]);
	}

	for (i = 0; i < cantBloquesArchivo; ++i)
		sem_wait(&pedidoTerminado);

	for (i = 0; i< cantidadNodos; ++i){
		printf("%d bloques fueron pedidos a %d\n", peticiones[i], indexNodos[i]);
	}

	free(currentChar);
	free(rutaArchivoEnMetadata);
	free(nombre);
	free(rutaFinal);
	fclose(informacionArchivo);
	return lectura;
}

void* leerDeDataNode(void* parametros){
	 struct parametrosLecturaBloque* params;
	 params = (struct parametrosLecturaBloque*) parametros;
	 sem_t* semaforo = list_get(pedidosFS,params->sem);
	 sem_wait(semaforo);
	 respuesta respuesta;
	 empaquetar(params->socket, mensajeNumeroLecturaBloqueANodo, sizeof(int),&params->bloque);
	 respuesta = desempaquetar(params->socket);
	 params->contenidoBloque = malloc(params->sizeBloque + 1);
	 memset(params->contenidoBloque, 0, params->sizeBloque + 1);
	 memcpy(params->contenidoBloque, respuesta.envio, params->sizeBloque);
	 sem_post(semaforo);
	 sem_post(&pedidoTerminado);
	 free(respuesta.envio);
	 pthread_exit(params->contenidoBloque);//(void*)params->contenidoBloque;
}

char* nombreArchivoSinExtension(char* nombre){
	char* nombreSinExtension = nombre;
	int indexNombre = 0;
	if (string_contains(nombreSinExtension,".")){
		char* reverse = string_reverse(nombre);
		char* currentChar = string_substring(nombre,indexNombre,1);
		while (strcmp(currentChar,".")){
			++indexNombre;
			free(currentChar);
			currentChar = string_substring(nombre,indexNombre,1);
		}
		free(currentChar);
		free(reverse);
		nombreSinExtension = string_substring(nombre,0,indexNombre);
	}
	return nombreSinExtension;
}

int guardarEnNodos(char* path, char* nombre, char* tipo, string* mapeoArchivo){

	int binario = strcmp(tipo, "b") == 0;

	char* ruta = buscarRutaArchivo(path);
	char* rutaFinal = malloc(strlen(ruta) + strlen(nombre) + 2);
	memset(rutaFinal, 0, strlen(ruta) + strlen(nombre) + 2);

	if(validarArchivo(string_from_format("%s/%s", ruta, nombre))){
		printf("El archivo ingresado no existe.\n");
		return 3;
	}

	if (!validarDirectorio(ruta))
		mkdir(ruta,0777);

	memcpy(rutaFinal, ruta, strlen(ruta));
	memcpy(rutaFinal + strlen(ruta), "/", 1);
	memcpy(rutaFinal + strlen(ruta) + 1, nombre, strlen(nombre));

	int sizeAux = strlen(mapeoArchivo->cadena);
	int cantBloquesArchivo = 0;

	int i, j, k, bloqueLibre = -1;
	while(sizeAux > 0){
		sizeAux -= mb;
		++cantBloquesArchivo;
	}
	//--cantBloquesArchivo;

	int bytesACortarAux[cantBloquesArchivo];
	int off = 0;
	int realTotal = cantBloquesArchivo;

	int sizeUltimoNodo = sizeAux+mb;
	int resTotal = 0;

	if (!binario){
		for (i = 0; i < cantBloquesArchivo-1; ++i){
			bytesACortarAux[i] = bytesACortar(mapeoArchivo->cadena,off,0);
			off += mb - bytesACortarAux[i];
			resTotal += bytesACortarAux[i];
		}

		resTotal += sizeUltimoNodo;
		if (resTotal > mb){
			realTotal += redondearHaciaArriba(resTotal,mb);
			--realTotal;
		}
	}

	int bytesACortarArray[realTotal];

	if (!binario){
		for(i = 0; i < cantBloquesArchivo-1; ++i){
			bytesACortarArray[i] = bytesACortarAux[i];
		}
		while (resTotal > mb){
			bytesACortarArray[i] = bytesACortar(mapeoArchivo->cadena,off,0);
			off += mb - bytesACortarArray[i];
			resTotal -= mb;
			resTotal += bytesACortarArray[i];
			++i;
		}
	}
	else
	{
		resTotal = sizeUltimoNodo;
	}



	//Busco la ruta donde tengo que guardar el archivo y lo dejo en blanco

	int offset = 0;
	int sizeRestante = 0;
	successArchivoCopiado = 1;
	int nodoAMandar = 0, nodoElegido[numeroCopiasBloque];
	nodoElegido[0] = -1;
	nodoElegido[1] = -1;

	printf("Bloques necesarios %d.\n",realTotal);
	//printf("%d %d bytes \n", realTotal, cantBloquesArchivo);
	informacionNodo infoAux;
	int cantidadNodos = list_size(nodosConectados);
	int bloquesLibreNodo[cantidadNodos];
	int indexNodos[cantidadNodos];
	parametrosEnvioBloque params[realTotal*2];
	int sizeTotal = 0, ultimoUtilizado = 0;
	int pruebaEspacioDisponible[cantidadNodos];
	int contadorPrueba = 0;
	int pruebaElegido[2];
	pruebaElegido[0] = -1;
	pruebaElegido[1] = -1;
	int envios[cantidadNodos];

	params[0].restanteAnterior = 0;
	params[realTotal].restanteAnterior = 0;
	for (i = 0; i < cantidadNodos; ++i){
		infoAux = *(informacionNodo*)list_get(nodosConectados,i);
		bloquesLibreNodo[i] = infoAux.sizeNodo-infoAux.bloquesOcupados;
		pruebaEspacioDisponible[i] = bloquesLibreNodo[i];
		indexNodos[i] = infoAux.numeroNodo;
		//printf("bloques libres %d\n", bloquesLibreNodo[i]);
	}
	int contador = 0;

	//printf("Size archivo %d.\n", strlen(mapeoArchivo->cadena));

	bla = realTotal*2;

	for (i= 0; i < cantidadNodos; ++i)
		envios[i] = 0;

	for (i = 0; i < realTotal; ++i){
		for (j = 0; j < cantidadNodos; ++j){
			if (contador > cantidadNodos -1)
				contador = 0;
			if(pruebaEspacioDisponible[contador] > 0 && pruebaElegido[0] != contador){
				if (contadorPrueba < 2){
					pruebaElegido[contadorPrueba] = contador;
					++contadorPrueba;
					--pruebaEspacioDisponible[contador];
					if (contadorPrueba >= 2)
						break;
				}
			}
			++contador;
		}
		if (contadorPrueba < 2){
			printf("No hay espacio suficiente.\n");
			return 2;
		}
		else{
			contadorPrueba = 0;
			pruebaElegido[0] = -1;
		    pruebaElegido[1] = -1;
		}
	}

	pthread_t nuevoHilo[realTotal*2];
	FILE* archivos = fopen(rutaFinal, "wb+");
	fclose(archivos); //para dejarlo vacio
	t_config* infoArchivo = config_create(rutaFinal);

	printf("Copiando\n");

	for (i = 0; i < realTotal; ++i){

		//printf("bl %d %d %d %d\n", bloquesLibreNodo[0],bloquesLibreNodo[1],bloquesLibreNodo[2],bloquesLibreNodo[3]);
		for (k = 0; k < numeroCopiasBloque; ++k){
			for (j = 0; j < cantidadNodos; ++j){
				if (nodoAMandar > cantidadNodos-1)
					nodoAMandar = 0;
				if(bloquesLibreNodo[nodoAMandar] > 0 && nodoElegido[0] != nodoAMandar){
					nodoElegido[k] = nodoAMandar;
					--bloquesLibreNodo[nodoAMandar];
					++envios[nodoAMandar];
					++nodoAMandar;
					break;
				}
				++nodoAMandar;

			}
		}

		for (j = 0; j < numeroCopiasBloque; ++j){

			params[i+realTotal*j].mapa = mapeoArchivo->cadena;
			params[i+realTotal*j].offset = offset;
			infoAux = *(informacionNodo*)list_get(nodosConectados,nodoElegido[j]);
			//printf("%d\n", nodoElegido[j]);

			if ( i > 0)
				params[i+realTotal*j].restanteAnterior = mb - params[i+realTotal*j-1].sizeBloque;
			else
				params[i+realTotal*j].restanteAnterior = 0;
			bloqueLibre = buscarPrimerBloqueLibre(nodoElegido[j], infoAux.sizeNodo);
			params[i+realTotal*j].socket = infoAux.socket;
			params[i+realTotal*j].bloque = bloqueLibre;

			if (i < realTotal-1){
				if (!binario)
				{
					if (j == 0){
						sizeRestante = bytesACortarArray[i];
					}
				}
				else
					sizeRestante = 0;

				 params[i+realTotal*j].sizeBloque = mb -sizeRestante;
				// printf("size bloque %s %d\n", tipo, params[i+realTotal*j].sizeBloque);
			 }
			 else{
				params[i+realTotal*j].sizeBloque = resTotal;
			 }
			if(j==0)
				sizeTotal += params[i+realTotal*j].sizeBloque;

			params[i+realTotal*j].sem = nodoElegido[j];
			pthread_create(&nuevoHilo[i+realTotal*j], NULL, &enviarADataNode,(void*) &params[i+realTotal*j]);
			pthread_join(nuevoHilo[i],NULL);

			if (successArchivoCopiado == 1){
				setearBloqueOcupadoEnBitmap(nodoElegido[j], bloqueLibre);
				config_set_value(infoArchivo, string_from_format("BLOQUE%dBYTES",i), string_itoa(params[i+realTotal*j].sizeBloque));
				config_set_value(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",i ,j), generarArrayBloque(indexNodos[nodoElegido[j]], bloqueLibre));
			}


			//printf("size res %d %d\n", infoAux.numeroNodo, bloqueLibre);
		}
		nodoElegido[0] = -1;
		nodoElegido[1] = -1;
		if (i < realTotal-1)
			offset += params[i].sizeBloque;
	}

	if(successArchivoCopiado == 1){ //Por cada bloque agrego sus valores para la tabla
		if (strcmp(path, "yamafs:/") == 0)
			config_set_value(infoArchivo, "RUTA", string_from_format("%s%s", path, nombre));
		else
			config_set_value(infoArchivo, "RUTA", string_from_format("%s/%s", path, nombre));

		config_set_value(infoArchivo, "TAMANIO", string_itoa(sizeTotal));
	}
	printf("Size total: %d.\n", sizeTotal);
	config_save_in_file(infoArchivo, rutaFinal); //guarda la tabla de archivos
	free(rutaFinal);

	for (i = 0; i < realTotal * 2; ++i){
		sem_wait(&pedidoTerminado);
	}

	for (i = 0; i< cantidadNodos; ++i){
		printf("%d bloques fueron enviados a %d.\n", envios[i], indexNodos[i]);
	}

	actualizarBitmapNodos();

	return successArchivoCopiado;

}

void* enviarADataNode(void* parametros){
	 struct parametrosEnvioBloque* params;
	 params = (struct parametrosEnvioBloque*) parametros;
	 int success;
	 //--bla;
	 //printf("bla %d %d\n", bla, params->sizeBloque);
	 pthread_mutex_lock(&listSemMutex);
	 sem_t* semaforo = (sem_t*)list_get(pedidosFS,params->sem);
	 pthread_mutex_unlock(&listSemMutex);
	 sem_wait(semaforo);
	 char* buff = malloc(params->sizeBloque + 1);
	 memset(buff,0, params->sizeBloque + 1);
	 memcpy(buff, params->mapa+params->offset, params->sizeBloque);
	 empaquetar(params->socket, mensajeNumeroCopiaBloqueANodo, sizeof(int),&params->bloque);
	 empaquetar(params->socket, mensajeEnvioBloqueANodo, params->sizeBloque,buff);
	 respuesta res = desempaquetar(params->socket);
	 memcpy(&success, res.envio, sizeof(int));
	 //printf("params %d %d %d\n", params->sizeBloque, params->bloque, res.idMensaje);
	 if (success == 0){
		 successArchivoCopiado = 0;
	 }
	 free(buff);
	 sem_post(semaforo);
	 sem_post(&pedidoTerminado);
	 return 0;
}

int bytesACortar(char* mapa, int offset, int sizeRestante){
	int index = 0;
	char* bloque = malloc(mb + 1);
	memset(bloque,0, mb+1);
	memcpy(bloque,mapa+offset-sizeRestante,mb);
	char* mapaInvertido = string_reverse(bloque);
	char currentChar;
	memcpy(&currentChar,mapaInvertido,1);
	while(currentChar != '\n'){
		++index;
		memcpy(&currentChar,mapaInvertido + index,1);
		if (index == mb)
			return 0;
	}
	free(bloque);
	free(mapaInvertido);
	return index;
}

//for(i = 0, );

void obtenerLinea(char* resultado, char* archivo, int offset){
	int index = 0;

	char* linea = malloc(50);
	memset(linea, 0, 50);

	char* caracter = string_substring(archivo, offset + index, 1);

	while(strcmp(caracter, "\n") != 0){
		memcpy(linea + index, caracter, 1);
		//linea = realloc(linea, strlen(linea)+1);
		++index;
		caracter = string_substring(archivo, offset + index, 1);
	}
	memset(resultado, 0, 50);
	memcpy(resultado, linea, strlen(linea));

	free(linea);
	free(caracter);
}

void setearBloqueOcupadoEnBitmap(int numeroNodo, int bloqueLibre){
	informacionNodo* infoAux;
	t_bitarray* bitarrayNodo = list_get(bitmapsNodos,numeroNodo);
	bitarray_set_bit(bitarrayNodo,bloqueLibre);
	infoAux = list_get(nodosConectados,numeroNodo);
	++infoAux->bloquesOcupados;
	--bloquesLibresTotales;
}

void setearBloqueLibreEnBitmap(int numeroNodo, int bloqueOcupado){
	informacionNodo* infoAux;
	t_bitarray* bitarrayNodo = list_get(bitmapsNodos,numeroNodo);
	bitarray_clean_bit(bitarrayNodo,bloqueOcupado);
	infoAux = list_get(nodosConectados,numeroNodo);
	--infoAux->bloquesOcupados;
	++bloquesLibresTotales;
}

bool esBloqueOcupado(int numeroNodo, int numeroBloque){

	t_bitarray* bitarrayNodo = list_get(bitmapsNodos,numeroNodo - 1);

	return bitarray_test_bit(bitarrayNodo,numeroBloque) == 1;
}

void actualizarBitmapNodos(){
	char* sufijo = ".bin";
	int cantidadNodos = list_size(nodosConectados);
	int i,j;

	t_bitarray* bitarrayNodo;
	informacionNodo infoAux;
	for (j = 0; j < cantidadNodos; j++){
		bitarrayNodo = list_get(bitmapsNodos,j);
		infoAux = *(informacionNodo*)list_get(nodosConectados,j);

		int longitudPath = strlen(rutaBitmaps);
		char* ruta = malloc(strlen(rutaBitmaps) + 1);
		memset(ruta, 0, strlen(rutaBitmaps) + 1);
		memcpy(ruta, rutaBitmaps, strlen(rutaBitmaps));
		char* nombreNodo = string_from_format("NODO%d",infoAux.numeroNodo);
		int longitudNombre = strlen(nombreNodo);
		int longitudSufijo = strlen(sufijo);
		char* pathParticular = malloc(longitudPath + longitudNombre + longitudSufijo + 1);
		memset(pathParticular,0,longitudPath + longitudNombre + longitudSufijo +1);

		memcpy(pathParticular, ruta, longitudPath);
		memcpy(pathParticular + longitudPath, nombreNodo, longitudNombre);
		memcpy(pathParticular + longitudPath + longitudNombre, sufijo, longitudSufijo);

		FILE* archivoBitmap = fopen(pathParticular, "wb+");

		for (i = 0; i < infoAux.sizeNodo; ++i){
			if (bitarray_test_bit(bitarrayNodo,i) == 0)
				fwrite("0",1,1,archivoBitmap);
			else
				fwrite("1",1,1,archivoBitmap);
		}

		actualizarArchivoNodos();
		fclose(archivoBitmap);
		free(pathParticular);
	}
}

char* generarArrayBloque(int numeroNodo, int numeroBloque){
	return string_from_format("[NODO%d,%d]",numeroNodo ,numeroBloque);
}

int buscarPrimerBloqueLibre(int numeroNodo, int sizeNodo){
	t_bitarray* bitarrayNodo = list_get(bitmapsNodos,numeroNodo);
	int i;
	for (i = 0; i < sizeNodo; ++i){
		if (bitarray_test_bit(bitarrayNodo,i) == 0){
			return i;
		}
	}
	return -1;
}

int levantarBitmapNodo(int numeroNodo, int sizeNodo) { //levanta el bitmap y a la vez devuelve la cantidad de bloques libres en el nodo
	char* sufijo = ".bin";
	t_bitarray* bitmap;

	int BloquesOcupados = 0, i;
	int longitudPath = strlen(rutaBitmaps);
	char* nombreNodo = string_from_format("NODO%d",numeroNodo);
	int longitudNombre = strlen(nombreNodo);
	int longitudSufijo = strlen(sufijo);

	char* pathParticular = malloc(longitudPath + longitudNombre + longitudSufijo + 1);
	memset(pathParticular, 0, longitudPath + longitudNombre + longitudSufijo + 1);

	char* espacioBitarray = malloc(sizeNodo+1);
	char* currentChar = malloc(sizeof(char)+1);
	memset(espacioBitarray, 0, sizeNodo + 1);
	memset(currentChar, 0, 2);
	int posicion = 0;

	memcpy(pathParticular, rutaBitmaps, longitudPath);
	memcpy(pathParticular + longitudPath, nombreNodo, longitudNombre);
	memcpy(pathParticular + longitudPath + longitudNombre, sufijo, longitudSufijo);

	FILE* bitmapFile;

	if (!validarArchivo(pathParticular) == 1){
		bitmapFile = fopen(pathParticular, "w");
		for(i = 0; i < sizeNodo; ++i)
			fwrite("0",1,1, bitmapFile);
		fclose(bitmapFile);
	}

	bitmapFile = fopen(pathParticular, "r");

	bitmap = bitarray_create_with_mode(espacioBitarray, sizeNodo, LSB_FIRST);

	fread(currentChar, 1, 1, bitmapFile);
	while (!feof(bitmapFile)) {
		if (strcmp(currentChar, "1") == 0){
			bitarray_set_bit(bitmap, posicion);
			++BloquesOcupados;
		}
		else
			bitarray_clean_bit(bitmap, posicion);
		++posicion;
		fread(currentChar, 1, 1, bitmapFile);
	}
	printf("ocupados %d\n", BloquesOcupados);

	free(pathParticular);
	fclose(bitmapFile);

	list_add(bitmapsNodos,bitmap);

	return BloquesOcupados;
}

void actualizarArchivoNodos(){

	char* pathArchivo = "../metadata/Nodos.bin";
	if (!recuperarEstado){
		FILE* archivoNodes = fopen(pathArchivo, "wb+");
		fclose(archivoNodes); //para dejarlo vacio
	}
	int cantidadNodos = list_size(nodosConectados), i = 0;
	int bloquesLibresNodo = 0;
	informacionNodo info;

	t_config* nodos = config_create(pathArchivo);

	sizeTotalNodos = 0;
	nodosLibres = 0;
	for (i = 0; i < cantidadNodos; ++i){
		info = *(informacionNodo*)list_get(nodosConectados,i);
		bloquesLibresNodo = info.sizeNodo-info.bloquesOcupados;
		sizeTotalNodos += info.sizeNodo;
		nodosLibres += bloquesLibresNodo;
		config_set_value(nodos, string_from_format("NODO%dTOTAL",info.numeroNodo), string_itoa(info.sizeNodo));
		config_set_value(nodos, string_from_format("NODO%dLIBRE",info.numeroNodo), string_itoa(bloquesLibresNodo));
	}

	config_set_value(nodos, "TAMANIO", string_itoa(sizeTotalNodos));

	config_set_value(nodos, "LIBRE", string_itoa(nodosLibres));

	if (!recuperarEstado){
		char* arrayNodos = generarArrayNodos();
		config_set_value(nodos, "NODOS", arrayNodos);
		free(arrayNodos);
	}

	config_save_in_file(nodos, pathArchivo);

}

char* generarArrayNodos(){
	int i, cantidadNodos = list_size(nodosConectados);
	int indexes[cantidadNodos];
	char* array;
	int longitudEscrito = 0;
	int longitudSting = 1 + cantidadNodos*5; // 2 [] + "NODO," * cantidad nodos - 1 de la ultima coma, despues se le suman los numeros
	informacionNodo info;
	for (i = 0; i < cantidadNodos; ++i){
		info = *(informacionNodo*)list_get(nodosConectados,i);
		indexes[i] = info.numeroNodo;
		longitudSting += strlen(string_itoa(info.numeroNodo));
	}
	array = malloc(longitudSting + 1);
	memset(array, 0,longitudSting + 1);
	memcpy(array,"[",1);
	++longitudEscrito;
	for (i = 0; i < cantidadNodos; ++i){
		char* temp = string_from_format("NODO%d,",indexes[i]);
		memcpy(array + longitudEscrito,  temp, strlen(temp));
		longitudEscrito += strlen(temp);
	}
	memcpy(array + longitudEscrito-1,"]",1);
	return array;
}



void atenderSolicitudYama(int socketYama, void* envio){



	//empaquetar(socketYama, mensajeInfoArchivo, 0 ,0);

	//aca hay que aplicar la super funcion mockeada de @Ronan

}

informacionNodo* informacionNodosConectados(){
	int cantidadNodos = list_size(nodosConectados);
	informacionNodo* listaInfoNodos = malloc(sizeof(informacionNodo)*cantidadNodos);

	int i;

	for (i = 0; i < cantidadNodos; i++){
		listaInfoNodos[i] = *(informacionNodo*) list_get(nodosConectados, i);
	}
	return listaInfoNodos;
}

informacionArchivoFsYama obtenerInfoArchivo(string rutaDatos){
	informacionArchivoFsYama info;
	info.informacionBloques = list_create();

	char* directorio = rutaSinArchivo(rutaDatos.cadena);
	char* rutaArchivo = buscarRutaArchivo(directorio);

	if(!strcmp(rutaArchivo, "-1")) {
		info.tamanioTotal = 0;
		return info;
	}

	char* rutaMetadata = string_from_format("%s/%s", rutaSinPrefijoYama(rutaArchivo), ultimaParteDeRuta(rutaDatos.cadena));

	t_config* archivo = config_create(rutaMetadata);

	info.tamanioTotal = config_get_int_value(archivo,"TAMANIO");

	int cantBloques = redondearHaciaArriba(info.tamanioTotal , sizeBloque);

	int i;
	for(i=0;i<cantBloques;i++){
		char* clave = calloc(1,8);
		string_append_with_format(&clave,"BLOQUE%d",i);
		infoBloque* infoBloqueActual = malloc(sizeof(infoBloque));
		char* claveCopia0 = strdup(clave);
		char* claveCopia1 = strdup(clave);
		char* claveBytes = strdup(clave);

		string_append(&claveCopia0,"COPIA0");
		string_append(&claveCopia1,"COPIA1");
		string_append(&claveBytes,"BYTES");

		infoBloqueActual->numeroBloque = i;

		infoBloqueActual->bytesOcupados = config_get_int_value(archivo,claveBytes);
		obtenerNumeroNodo(archivo,claveCopia0,&(infoBloqueActual->ubicacionCopia0));
		obtenerInfoNodo(&infoBloqueActual->ubicacionCopia0);

		obtenerNumeroNodo(archivo,claveCopia1,&(infoBloqueActual->ubicacionCopia1));
		obtenerInfoNodo(&infoBloqueActual->ubicacionCopia1);
		free(clave);
		list_add(info.informacionBloques,infoBloqueActual);
	}
	return info;
}

void obtenerInfoNodo(ubicacionBloque* ubicacion){
	int i;
	informacionNodo* info = malloc(sizeof(informacionNodo));
	for(i=0;i<list_size(nodosConectados);i++){
		info =(informacionNodo*)list_get(nodosConectados,i);

		if(info->numeroNodo == ubicacion->numeroNodo){
			ubicacion->ip.cadena = strdup(info->ip.cadena);
			ubicacion->ip.longitud = info->ip.longitud;
			ubicacion->puerto = info->puerto;
		}
	}
}

int guardarBloqueEnNodo(int bloque, int nodo, t_config* infoArchivo){
	int respuesta = 1, respuestaEnvio = -1;
	int i;
	int socketPedido = -1;
	int bloquePedido = -1;
	char** nodoPedido;
	int numeroNodoPedido = -1;
	int nodoElegido = -1;
	char* respuestaLectura;
	informacionNodo info;
	parametrosLecturaBloque paramLectura;
	parametrosEnvioBloque paramEnvio;
	pthread_t nuevoHilo;

	if (config_has_property(infoArchivo, string_from_format("BLOQUE%dBYTES",bloque))){
		paramLectura.sizeBloque = config_get_int_value(infoArchivo,string_from_format("BLOQUE%dBYTES",bloque));
	}else{
		return -1;
	}
	for (i = 0; i < list_size(nodosConectados); ++i){
		info = *(informacionNodo*)list_get(nodosConectados,i);
		if (info.numeroNodo == nodo){
			nodoElegido = i;
			break;
		}
	}
	int contador = 0;
	while (config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",bloque, contador)) && socketPedido == -1){
		nodoPedido = config_get_array_value(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",bloque, contador));
		numeroNodoPedido = atoi(string_substring_from(nodoPedido[0],4));
		for (i = 0; i < list_size(nodosConectados); ++i){
			info = *(informacionNodo*)list_get(nodosConectados,i);
			if (info.numeroNodo == numeroNodoPedido){
				socketPedido = info.socket;
				bloquePedido = atoi(nodoPedido[1]);
				break;
			}
		}
		free(nodoPedido);
		++contador;
	}

	if (socketPedido == -1){
		printf("Ningun nodo conectado tiene el bloque necesario\n");
		return -3;
	}


	if (nodoElegido == -1){
		return -2;
	}

	paramLectura.socket = socketPedido;
	paramLectura.bloque = bloquePedido;
	paramLectura.sem = nodoElegido;

	pthread_create(&nuevoHilo, NULL, &leerDeDataNode,(void*) &paramLectura);
	pthread_join(nuevoHilo, (void**) &respuestaLectura);

	//printf("sali de leer\n");

	paramEnvio.bloque = buscarPrimerBloqueLibre(nodoElegido, info.sizeNodo);
	paramEnvio.offset = 0;
	paramEnvio.restanteAnterior = 0;
	paramEnvio.sem = nodoElegido;
	paramEnvio.sizeBloque = paramLectura.sizeBloque;
	paramEnvio.socket = info.socket;
	paramEnvio.mapa = paramLectura.contenidoBloque;

	pthread_create(&nuevoHilo, NULL, &enviarADataNode,(void*) &paramEnvio);
	pthread_join(nuevoHilo, (void**) &respuestaEnvio);

	//printf("sali de enviar\n");

	setearBloqueOcupadoEnBitmap(nodoElegido, paramEnvio.bloque);

	free(paramEnvio.mapa);

	return paramEnvio.bloque;
}

int obtenerNumeroCopia(t_config* infoArchivo,int bloqueACopiar){
	int numeroCopiaBloqueNuevo = 0;

	if (config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",bloqueACopiar, 0)))
		numeroCopiaBloqueNuevo = 0;
	else
		return numeroCopiaBloqueNuevo;

	while (config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",bloqueACopiar, numeroCopiaBloqueNuevo))){
		++numeroCopiaBloqueNuevo;
	}

	return numeroCopiaBloqueNuevo;
}

int borrarDirectorios(){
	int i, respuesta = 1;
	char* root = "../metadata/Archivos/0";

	tablaDeDirectorios[0].index = 0;
	tablaDeDirectorios[0].padre = -1;
	memcpy(tablaDeDirectorios[0].nombre,"/",1);

	for (i = 1; i < 100; ++i){
		tablaDeDirectorios[i].index = -1;
		tablaDeDirectorios[i].padre = -1;
		memset(tablaDeDirectorios[i].nombre,0,255);
		memcpy(tablaDeDirectorios[i].nombre," ",1);
	}

	guardarTablaDirectorios();

	if(validarArchivo(pathArchivoDirectorios)){
		char* rmDirectorios = string_from_format("rm %s", pathArchivoDirectorios);

		respuesta = system(rmDirectorios);
		printf("\n");
		free(rmDirectorios);

		if (respuesta == 0)
			respuesta = 1;
	}

	if (!validarDirectorio(root))
		mkdir(root,0777);

	return respuesta;
}

int borrarArchivosEnMetadata(){
	int respuesta;

	char* rmComando = string_from_format("rm -r %s", rutaArchivos);

	respuesta = system(rmComando);

	printf("\n");

	char* mkdirComando = string_from_format("mkdir %s", rutaArchivos);

	respuesta = system(mkdirComando);

	printf("\n");

	if (respuesta == 0)
		return 1;

	return 0;
}

int liberarNodosConectados(){
	int cantNodosConectados = list_size(nodosConectados);
	int i;
	int numeroNodo, sizeNodo;
	informacionNodo nodo;
	int respuesta = 1;

	char* rmComando = string_from_format("rm -r %s", rutaMetadataBitmaps);

	respuesta = system(rmComando);

	char* mkdirComando = string_from_format("mkdir %s", rutaMetadataBitmaps);

	respuesta = system(mkdirComando);
	if (cantNodosConectados > 0){
		for (i = 0; i < cantNodosConectados; ++i){
			nodo = *(informacionNodo*) list_get(nodosConectados, i);
			numeroNodo = nodo.numeroNodo;
			sizeNodo = nodo.sizeNodo;
			levantarBitmapNodo(numeroNodo, sizeNodo);
			respuesta++;
			printf("Nodo%d formateado.\n", nodo.numeroNodo);
		}
	}
	if (respuesta == cantNodosConectados)
		return 1;

	return 0;
}

int formatearDataBins(){
 	int resultado = 0;
 	int i;
 	int cantNodos = list_size(nodosConectados);
 	informacionNodo* nodo;
 	respuesta res;

 	for (i = 0; i < cantNodos; ++i){
 		nodo = (informacionNodo*) list_get(nodosConectados,i);
 		empaquetar(nodo->socket, mensajeBorraDataBin, 0, 0);
 		res = desempaquetar(nodo->socket);
 		resultado += *(int*) res.envio;
 		printf("Nodo%d formateado\n", nodo->numeroNodo);
 	}

 	if (resultado == cantNodos)
 		return 1;


 	return 0;
 }

char* devolverRuta(char* comando, int posicionPalabra){
	char* copiaComando = malloc(strlen(comando)+1);
	memset(copiaComando,0, strlen(comando)+1);
	memcpy(copiaComando, comando,strlen(comando)+1);
	char* ruta = strtok(copiaComando, " ");
	int i;

	for (i = 0; i < posicionPalabra; ++i){
		ruta = strtok(NULL, " ");
	}
	//free(copiaComando);
	return ruta;
}



bool isDirectoryEmpty(char *dirname) {
  int n = 0;
  struct dirent *d;
  DIR *dir = opendir(dirname);
  if (dir == NULL)
    return false;
  while ((d = readdir(dir)) != NULL) {
    if(++n > 2)
      break;
  }
  closedir(dir);

  return n <= 2;
}


int esRutaDeYama(char* ruta){
	return string_starts_with(ruta, "yamafs:/");
}

int borrarDeArchivoMetadata(char* ruta, int bloque, int copia){
	char* camino = "archivoTemporario.txt";

	FILE* file = fopen(camino, "wb+");
	fclose(file);

	struct stat fileStat;
	if(stat(ruta,&fileStat) < 0){
		printf("no se pudo abrir\n");
		return 1;
	}
	int fd = open(ruta,O_RDWR);

	char* contenido = malloc(sizeof(char));

	contenido = mmap(NULL,fileStat.st_size,PROT_READ,MAP_SHARED,fd,0);

	t_config* configArchivo = config_create(ruta);
	t_config* configAux = config_create(camino);

	char* linea = malloc(50);
	int offset = 0;
	int longitud;
	int numCopia;
	int contador = config_keys_amount(configArchivo);
	int i;

	char* keyBloque = string_from_format("BLOQUE%dCOPIA%d", bloque, copia);

	for(i = 0; i < contador; i++){

		obtenerLinea(&*linea, contenido, offset);
		offset += strlen(linea) + 1;

		if (!string_starts_with(linea, keyBloque)){
			if (!string_starts_with(linea, string_from_format("BLOQUE%dCOPIA", bloque))){
				longitud = longitudAntesDelIgual(linea);
				config_set_value(configAux, string_substring_until(linea, longitud), string_substring_from(linea, longitud+1));
			}else{
				longitud = longitudAntesDelIgual(linea);
				numCopia = atoi(string_substring(linea, longitud-1, longitud));
				if (numCopia < copia){
					config_set_value(configAux, string_substring_until(linea, longitud), string_substring_from(linea, longitud+1));
				}else{
					config_set_value(configAux, string_from_format("BLOQUE%dCOPIA%d", bloque, numCopia-1),
							config_get_string_value(configArchivo, string_from_format("BLOQUE%dCOPIA%d", bloque, numCopia)));
				}

			}
		}
	}

	free(linea);

	config_save_in_file(configAux, ruta);

	if (munmap(contenido, fileStat.st_size) == -1)
	{
		close(fd);
		free(keyBloque);
		perror("Error un-mmapping the file");
		exit(EXIT_FAILURE);
	}
	char* rmCommand = string_from_format("rm %s", camino);
	system(rmCommand);

	close(fd);
	free(keyBloque);

	return 0;
}

int longitudAntesDelIgual(char* cadena){
	int longitud = 0;

	char* caracter = malloc(2);

	memset(caracter, 0, 2);

	caracter = string_substring(cadena, longitud, 1);

	while(strcmp(caracter, "=") != 0){
		longitud++;
		caracter = string_substring(cadena, longitud, 1);
	}

	return longitud;

}
