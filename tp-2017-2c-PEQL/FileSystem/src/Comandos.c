/*
 * Comandos.c
 *
 *  Created on: 7/9/2017
 *      Author: utnso
 */

#include "Comandos.h"
#include "FuncionesFS.h"

#define mb 1048576

extern bool fsFormateado;

int formatearFS(int flag){
	int resultado = 0;

	resultado += borrarDirectorios();

	resultado += borrarArchivosEnMetadata();

	resultado += liberarNodosConectados();

	//resultado += formatearDataBins(); NO VA

	if (resultado == 3 && !flag){
		fsFormateado = true;
		printf("FileSystem formateado correctamente.\n");
		return 0;
	}

	return 1;
}


int eliminarArchivo(char* comando){
	int sizeArchivo, sizeAux, cantBloquesArchivo = 0, i, j, k, numeroNodo, bloqueNodo, respuesta;
	char** arrayInfoBloque;
	char* rutaArchivoYamafs = devolverRuta(comando,1);
	int nodoAUsar = -1;
	informacionNodo info;

	if (validarArchivoYamaFS(rutaArchivoYamafs) == 0){
		printf("la ruta ingresada no pertence a yamafs.\n");
		return 2;
	}

	char* rutaDirectorioYamafs = rutaSinArchivo(rutaArchivoYamafs);

	char* rutaMetadata = buscarRutaArchivo(rutaDirectorioYamafs);
	if (strcmp(rutaMetadata, "-1") == 0){
		printf("No existe el directorio ingresado.\n");
		return 1;
	}

	char* nombreArchivo = ultimaParteDeRuta(rutaArchivoYamafs);

	if(!string_contains(nombreArchivo, ".")){
		printf("La ruta ingresada no pertenece a un archivo.\n");
		return 3;
	}

	char* rutaArchivoEnMetadata = malloc(strlen(rutaMetadata) + strlen(nombreArchivo) + 2);
	memset(rutaArchivoEnMetadata,0,strlen(rutaMetadata) + strlen(nombreArchivo) + 2);
	memcpy(rutaArchivoEnMetadata, rutaMetadata, strlen(rutaMetadata));
	memcpy(rutaArchivoEnMetadata + strlen(rutaMetadata), "/", 1);
	memcpy(rutaArchivoEnMetadata + strlen(rutaMetadata) + 1, nombreArchivo, strlen(nombreArchivo));

	if (!validarArchivo(rutaArchivoEnMetadata)){
		printf("No existe el archivo a eliminar.\n");
		return 1;
	}

	t_config* infoArchivo = config_create(rutaArchivoEnMetadata);

	if (config_has_property(infoArchivo, "TAMANIO"))
		sizeArchivo = config_get_int_value(infoArchivo,"TAMANIO");

	sizeAux = sizeArchivo;

	while(sizeAux > 0){
		sizeAux -= mb;
		++cantBloquesArchivo;
	}

	for (i = 0; i < cantBloquesArchivo; ++i){
		for (j = 0; j < numeroCopiasBloque; ++j){
			if (config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",i,j))){
				arrayInfoBloque = config_get_array_value(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",i,j));
				numeroNodo = atoi(string_substring_from(arrayInfoBloque[0], 4));
				for (k = 0; k < list_size(nodosConectados); ++k){
					info = *(informacionNodo*)list_get(nodosConectados, k);
					if (numeroNodo == info.numeroNodo)
						nodoAUsar = k;
				}
				bloqueNodo = atoi(string_substring_from(arrayInfoBloque[1], 0));
				setearBloqueLibreEnBitmap(nodoAUsar, bloqueNodo);
			}
		}
	}

	actualizarBitmapNodos();

	char* command = malloc(strlen(rutaArchivoEnMetadata) + 4);
	memset(command, 0, strlen(rutaArchivoEnMetadata) + 4);
	memcpy(command, "rm ", 3);
	memcpy(command + 3, rutaArchivoEnMetadata, strlen(rutaArchivoEnMetadata));

	respuesta = system(command);

	free(rutaArchivoEnMetadata);
	free(command);

	if (respuesta == 0){
		printf("Archivo eliminado correctamente.\n");
		return respuesta;
	}
	else{
		printf("No se pudo eliminar el archivo.\n");
		return respuesta;
	}
}


int eliminarDirectorio(char* comando){
	char* rutaDirectorioYamfs = devolverRuta(comando,2);

	if (validarArchivoYamaFS(rutaDirectorioYamfs) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 2;
	}

	char* rutaDirectorioMetadata = buscarRutaArchivo(rutaDirectorioYamfs);
	if (strcmp(rutaDirectorioMetadata, "-1") == 0){
		printf("El directorio ingresado no existe.\n");
		return 2;
	}
	int numeroTablaDirectorio = atoi(ultimaParteDeRuta(rutaDirectorioMetadata));

	if(numeroTablaDirectorio == 0){
		printf("No se puede eliminar root.\n");
		return 3;
	}

	if (isDirectoryEmpty(rutaDirectorioMetadata)){
		tablaDeDirectorios[numeroTablaDirectorio].index = -1;
		tablaDeDirectorios[numeroTablaDirectorio].padre = -1;
		memset(tablaDeDirectorios[numeroTablaDirectorio].nombre,0,255);
		memcpy(tablaDeDirectorios[numeroTablaDirectorio].nombre," ",1);

		if (strcmp(buscarRutaArchivo(rutaDirectorioYamfs), "-1") != 0)
			system(string_from_format("rm -d %s", buscarRutaArchivo(rutaDirectorioYamfs)));

		printf("Se elimino el directorio correctamente.\n");

		return 0;
	}else{
		printf("El directorio no esta vacio.\n");

		return 1;
	}
}


int eliminarBloque(char* comando){
	int respuesta = 1, otraCopia, numeroNodo, bloqueNodo, i, nodoElegido = -1;
	char** arrayInfoBloque;
	char* rutaArchivoYamafs = devolverRuta(comando,2);
	informacionNodo info;

	if (validarArchivoYamaFS(rutaArchivoYamafs) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 3;
	}

	int numeroBloque = atoi(devolverRuta(comando,3));
	int numeroCopia = atoi(devolverRuta(comando,4));


	if (strcmp(buscarRutaArchivo(rutaSinArchivo(rutaArchivoYamafs)), "-1") == 0){
		printf("El directorio ingresado no existe.\n");
		return 3;
	}

	char* rutaArchivoEnMetadata = string_from_format("%s/%s", buscarRutaArchivo(rutaSinArchivo(rutaArchivoYamafs)),
																				ultimaParteDeRuta(rutaArchivoYamafs));

	if (!validarArchivo(rutaArchivoEnMetadata)){
		printf("El archivo ingresado no existe.\n");
		return 3;
	}

	t_config* infoArchivo = config_create(rutaArchivoEnMetadata);

	if (numeroCopia == 0)
		otraCopia = 1;
	else
		otraCopia = 0;

	if (!config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",numeroBloque,numeroCopia))){
		printf("El bloque que quiere borrar no existe.\n");
		return 1;
	}

	if (!config_has_property(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",numeroBloque,otraCopia))){
		printf("El bloque que quiere borrar es la ultima copia.\n");
		return 2;
	}

	arrayInfoBloque = config_get_array_value(infoArchivo, string_from_format("BLOQUE%dCOPIA%d",numeroBloque,numeroCopia));
	numeroNodo = atoi(string_substring_from(arrayInfoBloque[0], 4));
	bloqueNodo = atoi(string_substring_from(arrayInfoBloque[1], 0));

	for (i = 0; i < list_size(nodosConectados); ++i){
		info = *(informacionNodo*)list_get(nodosConectados,i);
		if (info.numeroNodo == numeroNodo){
			nodoElegido = i;
			break;
		}
	}
	if (nodoElegido == -1){
		printf("No existe el nodo.\n");
		return 0;
	}



	setearBloqueLibreEnBitmap(nodoElegido, bloqueNodo);
	actualizarBitmapNodos();
	config_save(infoArchivo);

	respuesta = borrarDeArchivoMetadata(rutaArchivoEnMetadata, numeroBloque, numeroCopia);

	free(rutaArchivoEnMetadata);

	if (respuesta == 0){
		printf("Se elimino el bloque correctamente.\n");
		return respuesta;
	}
	else{
		printf("No se pudo eliminar el bloque.\n");
		return respuesta;
	}
}


int cambiarNombre(char* comando){

	char* rutaNombreViejo = devolverRuta(comando, 1);
	char* nombreNuevo = devolverRuta(comando, 2);


	int posicion = 0;
	int longitudNombreOriginal = 0;

	if (!string_starts_with(rutaNombreViejo,"yamafs:/")){
		printf("La ruta ingresada es invalida.\n");
		return 2;
	}

	rutaNombreViejo = rutaSinPrefijoYama(rutaNombreViejo);

	if(!string_contains(nombreNuevo, ".")){
		printf("Agregue la extension del nombre a cambiar.\n");
		return 3;
	}

	char* rutaNombreViejoReverse = string_reverse(rutaNombreViejo);
	char* caracterActual = string_substring(rutaNombreViejoReverse, posicion, 1);
	char* slash = "/";

	while(strcmp(caracterActual,slash)){
		++longitudNombreOriginal;
		++posicion;
		caracterActual = string_substring(rutaNombreViejoReverse, posicion, 1);
	}
	char* rutaSinNombre = string_substring(rutaNombreViejo, 0, strlen(rutaNombreViejo) - posicion);
	char* nombre = string_substring(rutaNombreViejo, strlen(rutaNombreViejo) - posicion, posicion);

	char* ruta = buscarRutaArchivo(rutaSinNombre);

	if(strcmp(ruta, "-1") == 0){
		printf("El directorio ingresado no existe.\n");
		return 2;
	}

	rutaNombreViejoReverse = string_substring_from(rutaNombreViejoReverse, longitudNombreOriginal + 1 );
	rutaNombreViejoReverse = string_reverse(rutaNombreViejoReverse);
	int tamanioRutaNueva = strlen(rutaNombreViejoReverse) + strlen(slash) + strlen(nombreNuevo);
	char* rutaNuevaDefinitiva = malloc(tamanioRutaNueva + 1);
	memset(rutaNuevaDefinitiva, 0, tamanioRutaNueva + 1);
	memcpy(rutaNuevaDefinitiva, rutaNombreViejoReverse, strlen(rutaNombreViejoReverse));
	memcpy(rutaNuevaDefinitiva + strlen(rutaNombreViejoReverse), slash, strlen(slash));
	memcpy(rutaNuevaDefinitiva + strlen(rutaNombreViejoReverse) + strlen(slash), nombreNuevo, strlen(nombreNuevo) + 1);

	char* nuevo = string_from_format("%s/%s", ruta, nombreNuevo);

	if (!validarArchivo(nuevo)){
		printf("El archivo no existe.\n");
		return 0;
	}

	char* nombreViejo = string_from_format("%s/%s", ruta, nombre);

	t_config* c = config_create(nombreViejo);
	config_set_value(c, "RUTA", string_from_format("%s%s", rutaSinNombre, nombre));
	config_save_in_file(c, nombreViejo);
	config_destroy(c);

	int resultado = rename(nombreViejo,nuevo);

	free(rutaNombreViejoReverse);
	free(caracterActual);
	free(rutaNuevaDefinitiva);
	free(nuevo);
	free(nombre);
	free(nombreViejo);
	free(rutaSinNombre);

	if (resultado == 0){
		printf("Archivo renombrado correctamente.\n");
		return resultado;
	}
	else{
		printf("No se pudo renombrar el resultado\n");
		return resultado;
	}
}


int mover(char* comando){
	char* slash = "/";
	char* dot = ".";


	char** arguments = string_split(comando, " ");

//	if(strcmp(arguments[1], "yamafs:/") == 0){
//		printf("")
//		return 3;
//	}

	if(!esRutaDeYama(arguments[1]) || !esRutaDeYama(arguments[2])){
		printf("La ruta ingresada es invalida.\n");
		return 2;
	}

	int indice = 0;
	char* rutaInvertida = string_reverse(arguments[1]);
	char* caracterActual = string_substring(rutaInvertida, indice, 1);
	int tipo = 0; //archivo

	while(strcmp(caracterActual,dot) != 0 && tipo == 0){
		++indice;
		caracterActual = string_substring(rutaInvertida, indice, 1);
		if (strcmp(caracterActual,slash) == 0){
			tipo = 1; //directorio
		}
	}

	int length = 0;
	if(strcmp(arguments[2], "yamafs:/") != 0){
		char* rutaInvertidaNuevaDir = string_reverse(arguments[2]);
		while(strcmp(caracterActual,slash) ){
			++length;
			caracterActual = string_substring(rutaInvertidaNuevaDir, length, 1);
			if (strcmp(caracterActual,dot) == 0){
				printf("Ruta invalida, es un archivo.\n");
				return 3;
			}
		}
	}

	int success = 0;
	if(tipo == 1){
		int indexDir = getIndexDirectorio(rutaSinPrefijoYama(arguments[1]));
		int indexDirPadre = getIndexDirectorio(rutaSinPrefijoYama(arguments[2]));
		if (indexDir == -1 || indexDirPadre == -1){
			printf("El directorio ingresado es invalido.\n");
			return 2;
		}
		tablaDeDirectorios[indexDir].padre = indexDirPadre;
		success = 0;
		guardarTablaDirectorios();
		printf("Directorio movido correctamente.\n");
	}
	else{
		char* rutaAnterior;
		char* rutaNueva = buscarRutaArchivo(arguments[2]);
		rutaAnterior = buscarRutaArchivo(rutaSinArchivo(arguments[1]));
		if(strcmp(rutaAnterior, "yamafs:") == 0){
			rutaAnterior = string_from_format("%s/",rutaAnterior);
		}

		char** partesRutaAnterior = string_split(arguments[1],"/");
		int i = 0;
		while(partesRutaAnterior[i] != NULL)
			++i;
		char* nombreArchivo = partesRutaAnterior[i-1];
		char* rutaFinalAnterior = string_from_format("%s/%s", rutaAnterior, nombreArchivo);
		if (!validarArchivo(rutaFinalAnterior)){
			printf("El archivo ingresado no existe.\n");
			return 3;
		}
		t_config* c = config_create(rutaFinalAnterior);
		config_set_value(c, "RUTA", string_from_format("%s/%s", arguments[2], nombreArchivo));
		config_save_in_file(c, rutaFinalAnterior);
		config_destroy(c);
		success = system(string_from_format("mv %s %s", rutaFinalAnterior, rutaNueva));
		if (success == 0)
			printf("Archivo movido correctamente.\n");
		else
			printf("No se pudo mover el archivo.\n");
	}
	//free(arguments);

	return success;
}


int mostrarArchivo(char* comando){

	int respuesta = 1;
	char* rutaArchivoYamafs = devolverRuta(comando,1);

	if (validarArchivoYamaFS(rutaArchivoYamafs) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 1;
	}

	char* rutaYamafs = rutaSinArchivo(rutaArchivoYamafs);
	char* rutaMetadata = buscarRutaArchivo(rutaYamafs);

	if (strcmp(rutaMetadata, "-1") == 0){
		printf("No existe el directorio.\n");
		return respuesta;
	}

	if (!validarArchivo(string_from_format("%s/%s", rutaMetadata, ultimaParteDeRuta(rutaArchivoYamafs)))){
		printf("No existe el archivo.\n");
		return 1;
	}

	char* contenido = leerArchivo(rutaArchivoYamafs);
	printf("%s\n", contenido);
	respuesta = 0;
	printf("Archivo mostrado correctamente.\n");
	free(contenido);

	return respuesta;
}


int crearDirectorio(char* comando){
	int respuesta = 1, i = 0;
	char* pathComando = devolverRuta(comando, 1);
	char* path;
	char* rutaPadre;
	int indexPadre = 0;
	char* nombre;
	int success = 1;

	if (validarArchivoYamaFS(pathComando) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 2;
	}

	path = rutaSinPrefijoYama(pathComando);
	if (strcmp("/", path) == 0){
		printf("No se creo el directorio, el directorio no puede ser root.\n");
		return 2;
	}

	respuesta = getIndexDirectorio(path);
	//printf("success %d\n", respuesta);

	if (respuesta == -1){
		rutaPadre = rutaSinArchivo(path);
		indexPadre = getIndexDirectorio(rutaSinPrefijoYama(rutaPadre));
		if (indexPadre == -1){
			printf("No existe ruta %s.\n", rutaPadre);
			return 1;
		}
		else{
			while(tablaDeDirectorios[i].index != -1){
				++i;
			}
			if (i == 100){
				printf("Se alcanzo el limite de directorios.\n");
				return 2;
			}

			success = 0;
			tablaDeDirectorios[i].index = i;
			tablaDeDirectorios[i].padre = indexPadre;
			nombre = ultimaParteDeRuta(path);
			memcpy(tablaDeDirectorios[i].nombre, nombre, strlen(nombre));
			guardarTablaDirectorios();
			mkdir(string_from_format("../metadata/Archivos/%d", i),0777);
			printf("Directorio creado correctamente.\n");
		}

	}
	else
		printf("Ya existe el directorio.\n");

	return success;
}


int copiarArchivo(char* comando){

	char** parametros = malloc(strlen(comando));

	parametros = string_split(comando, " ");
	char* rutaNormal = parametros[1];
	char* rutaFS = parametros[2];
	char* tipoArchivo = "t";

	if (parametros[3] != NULL)
		tipoArchivo = parametros[3];

	char* nombre = malloc(strlen(comando)-4); //El peor caso seria que el parametro sea el nombre sin ruta, tomo ese valor
	memset(nombre,0,strlen(comando)-4);

	char* rutaMetadata = buscarRutaArchivo(rutaFS);
	if (strcmp(rutaMetadata, "-1") == 0){
		printf("El directorio ingresado no existe.\n");
		return 0;
	}

	//printf("%s\n", rutaFS);
	if (!string_starts_with(rutaFS,"yamafs:/")){
		printf("La ruta ingresada es invalida.\n");
		return 0;
	}

	char* rutaFSMetadata = buscarRutaArchivo(rutaSinPrefijoYama(rutaFS));
	if (strcmp(rutaFSMetadata, "-1") == 0){
		printf("El directorio ingresado no existe.\n");
		return 0;
	}


	struct stat fileStat;
	if(stat(rutaNormal,&fileStat) < 0){
		printf("No se pudo abrir el archivo\n");
		free(nombre);
		return 0;
	}


	int fd = open(rutaNormal,O_RDWR);
	int size = fileStat.st_size;

	if (!S_ISREG(fileStat.st_mode)){
		printf("La ruta no pertenece a un archivo.\n");
		return 0;
	}

	if (size == 0){
		printf("El archivo esta vacio.\n");
		return 0;
	}
	nombre = ultimaParteDeRuta(rutaNormal);

	string* mapeoArchivo;

	mapeoArchivo = malloc(sizeof(string));
	mapeoArchivo->cadena = mmap(NULL,size,PROT_READ,MAP_SHARED,fd,0);
	mapeoArchivo->longitud = size;

	int resultado = guardarEnNodos(rutaFS, nombre, tipoArchivo, mapeoArchivo);

	if (munmap(mapeoArchivo->cadena, mapeoArchivo->longitud) == -1)
	{
		close(fd);
		perror("Error un-mmapping the file");
		exit(EXIT_FAILURE);
	}

	free(nombre);
	free(mapeoArchivo);
	free(parametros);

	if (resultado == 1){
		printf("Archivo copiado a yamafs.\n");
		return resultado;
	}
	else{
		printf("No se pudo copiar el archivo a yamafs.\n");
		return resultado;
	}
}


int copiarArchivoAFs(char* comando){
	int respuesta = 1;
	char* rutaArchivoYamafs = devolverRuta(comando,1);

	if (validarArchivoYamaFS(rutaArchivoYamafs) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 1;
	}

	char* directorioYamafs = rutaSinArchivo(rutaArchivoYamafs);
	char* rutaMetadata = buscarRutaArchivo(directorioYamafs);
	if (strcmp(rutaMetadata, "-1") == -0){
		printf("El directorio no existe.\n");
		return respuesta;
	}

	if(!validarArchivo(string_from_format("%s/%s", rutaMetadata,ultimaParteDeRuta(rutaArchivoYamafs)))){
		printf("El archivo no existe.\n");
		return 1;
	}

	char* contenido = leerArchivo(rutaArchivoYamafs);
	char* nombre = ultimaParteDeRuta(rutaArchivoYamafs);
	char* rutaDirFs = devolverRuta(comando,2);

	char* rutaFinal = string_from_format("%s/%s", rutaDirFs, nombre);

	FILE* archivo = fopen(rutaFinal, "w");
	fwrite(contenido, strlen(contenido), 1, archivo);

	fclose(archivo);
	free(contenido);
	free(rutaFinal);
	respuesta = 0;

	printf("Archivo copiado correctamente.\n");

	return respuesta;
}


int copiarBloqueANodo(char* comando){
	int respuesta = 1, bloqueNuevo, numeroCopiaBloqueNuevo;

	char* rutaArchivoYamafs = devolverRuta(comando,1);

	if (validarArchivoYamaFS(rutaArchivoYamafs) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 1;
	}

	int bloqueACopiar = atoi(devolverRuta(comando,2));
	int nodoACopiar = atoi(devolverRuta(comando,3));

	char* nombreArchivo = ultimaParteDeRuta(rutaArchivoYamafs);
	char* rutaDirectorioMetadata = buscarRutaArchivo(rutaSinArchivo(rutaArchivoYamafs));
	if (atoi(rutaDirectorioMetadata) == -1)
		return respuesta;

	char* rutaArchivoMetadata = string_from_format("%s/%s", rutaDirectorioMetadata, nombreArchivo);
	if (!validarArchivo(rutaArchivoMetadata)){
		printf("No existe el archivo.\n");
		return respuesta;
	}
	t_config* infoArchivo = config_create(rutaArchivoMetadata);

	bloqueNuevo = guardarBloqueEnNodo(bloqueACopiar, nodoACopiar, infoArchivo);

	if (bloqueNuevo == -1){
		printf("No existe el bloque\n");
		return respuesta;
	}
	if (bloqueNuevo == -2){
		printf("No existe el nodo\n");
		return respuesta;
	}


	numeroCopiaBloqueNuevo = obtenerNumeroCopia(infoArchivo, bloqueACopiar);

	config_set_value(infoArchivo, string_from_format("BLOQUE%dCOPIA%d", bloqueACopiar, numeroCopiaBloqueNuevo),
			string_from_format(generarArrayBloque(nodoACopiar,bloqueNuevo)));

	config_save(infoArchivo);

	actualizarBitmapNodos();
	respuesta = 0;

	config_destroy(infoArchivo);

	printf("Bloque copiado correctamente.\n");

	return respuesta;
}


int generarArchivoMD5(char* comando){
	int respuesta = 1;

	char* rutaArchivoYamafs = devolverRuta(comando,1);

	if (validarArchivoYamaFS(rutaArchivoYamafs) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 1;
	}
	char* directorioYamafs = rutaSinArchivo(rutaArchivoYamafs);
	char* rutaMetadata = buscarRutaArchivo(directorioYamafs);
	if (strcmp(rutaMetadata, "-1") == 0){
		printf("El directorio no existe.\n");
		return respuesta;
	}
	char* nombreArchivo = ultimaParteDeRuta(rutaArchivoYamafs);
	char* rutaArchivoMetadata = string_from_format("%s/%s", rutaMetadata, nombreArchivo);

	if (!validarArchivo(rutaArchivoMetadata)){
		printf("No existe el archivo.\n");
		return respuesta;
	}

	char* contenido = leerArchivo(rutaArchivoYamafs);

	char* ubicacionArchivoTemporal = string_from_format("%s", nombreArchivo);
	FILE* file = fopen(ubicacionArchivoTemporal, "w");
	fwrite(contenido, strlen(contenido), 1, file);
	fclose(file);
	free(contenido);

	char* MD5 = string_from_format("md5sum %s", nombreArchivo);
	char* RM = string_from_format("rm %s", nombreArchivo);

	respuesta = system(MD5);
	printf("\n");
	if (respuesta == 0){
		printf("MD5 generado correctamente.\n");
	}
	else{
		printf("No se pudo generar el archivo ");
	}
	system(RM);
	printf("\n");
	free(MD5);
	free(RM);
	free(ubicacionArchivoTemporal);
	free(rutaArchivoMetadata);

	return 0;

}


int listarArchivos(char* comando){
	int respuesta = 1;
	char* rutaYamafs = devolverRuta(comando, 1);

	if (validarArchivoYamaFS(rutaYamafs) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 1;
	}

	char* rutaFsLocal = buscarRutaArchivo(rutaYamafs);
	if (rutaFsLocal == string_itoa(-1)){
		printf("El directorio no existe.\n");
		return respuesta;
	}

	char* command = string_from_format("ls %s", rutaFsLocal);

	respuesta = system(command);

	if (respuesta == 0)
		printf("Archivos listados correctamente.\n");
	else
		printf("No se pudo listar los archivos.\n");

	free(command);

	return respuesta;
}


int informacion(char* comando){
	int respuesta = 1;
	char* rutaArchivoYamafs = devolverRuta(comando, 1);

	if (validarArchivoYamaFS(rutaArchivoYamafs) == 0){
		printf("La ruta ingresada es invalida.\n");
		return 1;
	}

	char* rutaDirectorioYamafs = rutaSinArchivo(rutaArchivoYamafs);
	char* nombreArchivo = ultimaParteDeRuta(rutaArchivoYamafs);
	char* rutaDirectorioMetadata = buscarRutaArchivo(rutaDirectorioYamafs);

	if (atoi(rutaDirectorioMetadata) == -1){
		printf("El directorio no existe.\n");
		return respuesta;
	}

	char* rutaArchivoMetadata = string_from_format("%s/%s", rutaDirectorioMetadata, nombreArchivo);

	if (!validarArchivo(rutaArchivoMetadata)){
		printf("El archivo no existe.\n");
		return respuesta;
	}

	char* command = string_from_format("cat %s", rutaArchivoMetadata);

	respuesta = system(command);

	if (respuesta == 0)
		printf("Información del archivo mostrada correctamente.\n");
	else
		printf("No se pudo mostrar la información del archivo.\n");

	free(command);
	free(rutaArchivoMetadata);

	return respuesta;
}
