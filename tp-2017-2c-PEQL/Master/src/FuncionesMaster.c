#include "FuncionesMaster.h"
#include <sys/mman.h>
#include <Globales.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

void conectarseConYama(char* ip, int port) {
	socketYama = crearSocket();
	struct sockaddr_in direccion = cargarDireccion(ip, port);
	conectarCon(direccion, socketYama, 2); //2 id master
	respuesta respuestaHandShake = desempaquetar(socketYama);

	if(respuestaHandShake.idMensaje != mensajeOk){
		log_error(loggerMaster, "Conexion fallida con YAMA");
		perror("Conexion fallida con YAMA");
		log_destroy(loggerMaster);
		exit(1);
	}
	log_trace(loggerMaster, "Conexion con Yama establecida");
}


job* crearJob(char* argv[]){
	job* nuevo = (job*)malloc(sizeof(job));

	nuevo->id = 0;
	nuevo->socketFd = 0;

	if( access( argv[2], F_OK ) == -1 ) {
	  finalizarJob(FALLO_INGRESO);
	}

	if( access( argv[3], F_OK ) == -1 ) {
		finalizarJob(FALLO_INGRESO);
	}

	nuevo->rutaTransformador.cadena = string_duplicate(argv[2]);
	nuevo->rutaTransformador.longitud = string_length(nuevo->rutaTransformador.cadena);

	nuevo->rutaReductor.cadena = string_duplicate(argv[3]);
	nuevo->rutaReductor.longitud = string_length(nuevo->rutaReductor.cadena);

	nuevo->rutaDatos.cadena= string_duplicate(argv[4]);
	nuevo->rutaDatos.longitud = string_length(nuevo->rutaDatos.cadena);

	nuevo->rutaResultado.cadena = string_duplicate(argv[5]);
	nuevo->rutaResultado.longitud = string_length(nuevo->rutaResultado.cadena);

	estadisticas = crearEstadisticasProceso();
	return nuevo;
}

void esperarInstruccionesDeYama() {
	respuesta instruccionesYama;
	respuestaSolicitudTransformacion* infoTransformacion ;
	nodosRedLocal* infoRedLocal;
	respuestaReduccionGlobal* infoRedGlobal;
	workerDesdeYama* worker;
	respuestaAlmacenamiento* almacenamiento;
	int fallo;

	while (1) {
		instruccionesYama = desempaquetar(socketYama);
		switch (instruccionesYama.idMensaje) {

			case mensajeRespuestaTransformacion:
				infoTransformacion = (respuestaSolicitudTransformacion*)instruccionesYama.envio;
				log_trace(loggerMaster, "Recibo Transformacion de YAMA.");
				inicializarTiemposTransformacion(infoTransformacion);
				crearHilosConexionTransformacion(infoTransformacion);
				break;

			case mensajeRespuestaRedLocal:
				infoRedLocal = (nodosRedLocal*)instruccionesYama.envio;
				log_trace(loggerMaster, "Recibo Reduccion Local en nodo %d de YAMA.",infoRedLocal->numeroNodo);
				crearHilosConexionRedLocal(infoRedLocal);
				break;

			case mensajeDesconexion:
				log_error(loggerMaster, "Error inesperado al recibir instrucciones de YAMA.");
				exit(1);
				break;

			case mensajeReplanificacion:
				worker= instruccionesYama.envio;
				log_trace(loggerMaster, "Recibo Replanificacion para bloque en nodo %d de YAMA.",worker->numeroWorker);
				crearHilosPorBloqueTransformacion(worker);
				break;

			case mensajeFinJob:
				fallo = *(int*)instruccionesYama.envio;
				log_trace(loggerMaster, "Finaliza job.");
				finalizarJob(fallo);
				break;

			case mensajeRespuestaRedGlobal:
				infoRedGlobal = (respuestaReduccionGlobal*)instruccionesYama.envio;
				log_trace(loggerMaster, "Recibo Reduccion Global en nodo %d de YAMA.",infoRedGlobal->numero);
				enviarAEncargadoRedGlobal(infoRedGlobal);
				break;

			case mensajeRespuestaAlmacenamiento:
				almacenamiento = (respuestaAlmacenamiento*)instruccionesYama.envio;
				log_trace(loggerMaster, "Recibo Almacenamiento Final en nodo %d de YAMA.",almacenamiento->nodo);
				enviarAlmacenamientoFinal(almacenamiento);
				break;
		}
	}
}

void crearHilosConexionTransformacion(respuestaSolicitudTransformacion* rtaYama) {
	int i;

	for(i=0 ; i<list_size(rtaYama->workers);i++){
		workerDesdeYama* worker = list_get(rtaYama->workers, i);
		crearHilosPorBloqueTransformacion(worker);
	}
}

void crearHilosPorBloqueTransformacion(workerDesdeYama* worker){
	int j;
	for(j=0 ; j<list_size(worker->bloquesConSusArchivos);j++){
		parametrosTransformacion* parametrosConexion = malloc(sizeof(parametrosTransformacion));
		parametrosConexion->ip.cadena = worker->ip.cadena;
		parametrosConexion->ip.longitud = worker->ip.longitud;
		parametrosConexion->numero = worker->numeroWorker;
		parametrosConexion->puerto = worker->puerto;
		bloquesConSusArchivosTransformacion* bloque = list_get(worker->bloquesConSusArchivos, j);
		parametrosConexion->bloquesConSusArchivos = *bloque;

		pthread_t nuevoHilo;
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);


		if (pthread_create(&nuevoHilo, &attr, (void *) conectarseConWorkersTransformacion, parametrosConexion) != 0) {
			log_error(loggerMaster, "No se pudo crear el thread de conexion");
			exit(-1);
		}
	}
}

void* conectarseConWorkersTransformacion(void* params) {
	parametrosTransformacion* infoTransformacion= (parametrosTransformacion*)params;
	respuesta confirmacionWorker;
	bloqueYNodo* bloqueOK;

	int socketWorker = crearSocket();
	struct sockaddr_in direccion = cargarDireccion(infoTransformacion->ip.cadena,infoTransformacion->puerto);
	if(!conectarCon(direccion, socketWorker, 2)){//2 id master
		mandarAReplanificar(infoTransformacion);
		return 0;

	}

	log_trace(loggerMaster, "Conexion con Worker %d para bloque %d", infoTransformacion->numero, infoTransformacion->bloquesConSusArchivos.numBloque);

	struct stat fileStat;
	if(stat(miJob->rutaTransformador.cadena,&fileStat) < 0){
		printf("No se pudo abrir el archivo\n");
		return 0;
	}

	int fd = open(miJob->rutaTransformador.cadena,O_RDWR);
	int size = fileStat.st_size;

	infoTransformacion->contenidoScript.cadena = mmap(NULL,size,PROT_READ,MAP_SHARED,fd,0);
	infoTransformacion->contenidoScript.longitud = size;

	empaquetar(socketWorker, mensajeProcesarTransformacion, 0, infoTransformacion);

	confirmacionWorker = desempaquetar(socketWorker);

	if (munmap(infoTransformacion->contenidoScript.cadena, infoTransformacion->contenidoScript.longitud) == -1){
		perror("Error un-mmapping the file");
		exit(EXIT_FAILURE);
	}
	close(fd);

	switch(confirmacionWorker.idMensaje){

		case mensajeTransformacionCompleta:
			log_trace(loggerMaster, "Informo a YAMA fin Transformacion para bloque %d en nodo %d.",infoTransformacion->bloquesConSusArchivos.numBloque,infoTransformacion->numero);
			bloqueOK = malloc(sizeof(bloqueYNodo));
			bloqueOK->workerId = infoTransformacion->numero;
			bloqueOK->bloque = infoTransformacion->bloquesConSusArchivos.numBloque;
			empaquetar(socketYama, mensajeTransformacionCompleta, 0 , bloqueOK);
			finalizarTiempo(estadisticas->tiempoFinTrans,bloqueOK->bloque);
			estadisticas->cantTareas[TRANSFORMACION]++;
			free(bloqueOK);
			break;

		case mensajeDesconexion:
			mandarAReplanificar(infoTransformacion);
			break;


	}
	return 0;
}
void crearHilosConexionRedLocal(nodosRedLocal* worker){

	parametrosReduccionLocal* parametrosConexion = malloc(sizeof(parametrosReduccionLocal));
	parametrosConexion->ip.cadena = strdup(worker->ip.cadena);
	parametrosConexion->ip.longitud = worker->ip.longitud;
	parametrosConexion->numero = worker->numeroNodo;
	parametrosConexion->puerto = worker->puerto;
	parametrosConexion->rutaDestino.longitud = worker->archivoTemporal.longitud;
	parametrosConexion->rutaDestino.cadena = strdup(worker->archivoTemporal.cadena);
	parametrosConexion->archivosTemporales = list_create();
	list_add_all(parametrosConexion->archivosTemporales,worker->archivos);

	setearTiempo(RED_LOCAL,worker->numeroNodo);

	pthread_t nuevoHilo;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);


	if (pthread_create(&nuevoHilo, &attr, (void*)conectarseConWorkersRedLocal, parametrosConexion) != 0) {
		log_error(loggerMaster, "No se pudo crear el thread de conexion");
		exit(-1);
	}

}

void* conectarseConWorkersRedLocal(void* params){
	parametrosReduccionLocal* infoRedLocal= (parametrosReduccionLocal*)params;
	respuesta confirmacionWorker;
	int numeroNodo;

	int socketWorker = crearSocket();
	struct sockaddr_in direccion = cargarDireccion(infoRedLocal->ip.cadena,infoRedLocal->puerto);
	if(!conectarCon(direccion, socketWorker, 2)){//2 id master
		mandarFalloEnReduccion();
		return 0;

	}

	log_trace(loggerMaster, "Conexion con Worker %d para estos tmp %d", infoRedLocal->numero, list_size(infoRedLocal->archivosTemporales));

	struct stat fileStat;
	if(stat(miJob->rutaReductor.cadena,&fileStat) < 0){
		printf("No se pudo abrir el archivo\n");
		return 0;
	}

	int fd = open(miJob->rutaReductor.cadena,O_RDWR);
	int size = fileStat.st_size;

	infoRedLocal->contenidoScript.cadena = mmap(NULL,size,PROT_READ,MAP_SHARED,fd,0);
	infoRedLocal->contenidoScript.longitud = size;

	empaquetar(socketWorker, mensajeProcesarRedLocal, 0, infoRedLocal);

	confirmacionWorker = desempaquetar(socketWorker);

	if (munmap(infoRedLocal->contenidoScript.cadena, infoRedLocal->contenidoScript.longitud) == -1){
		perror("Error un-mmapping the file");
		exit(EXIT_FAILURE);
	}
	close(fd);

	switch(confirmacionWorker.idMensaje){

	case mensajeRedLocalCompleta:
		log_trace(loggerMaster, "Informo a  YAMA fin Reduccion Local en nodo %d.",infoRedLocal->numero);
		numeroNodo = *(int*)confirmacionWorker.envio;
		empaquetar(socketYama, mensajeRedLocalCompleta, 0 , &numeroNodo);
		finalizarTiempo(estadisticas->tiempoFinRedLocal,numeroNodo);
		estadisticas->cantTareas[RED_LOCAL]++;
		break;

	case mensajeDesconexion:
	case mensajeFalloRedLocal:
		log_trace(loggerMaster, "Informo a  YAMA fallo en Reduccion Local en nodo %d.",infoRedLocal->numero);
		estadisticas->cantFallos++;
		mandarFalloEnReduccion();
		break;
	}
	return 0;
}

void mandarFalloEnReduccion(){
	estadisticas->cantFallos++;
	empaquetar(socketYama,mensajeFalloReduccion,0,0);
}

void mandarAReplanificar(parametrosTransformacion* infoTransformacion){
	log_trace(loggerMaster, "Informo a  YAMA replanificacion nodo %d.",infoTransformacion->numero);
	bloqueYNodo* bloqueReplanificar=malloc(sizeof(bloqueYNodo));
	bloqueReplanificar->workerId = infoTransformacion->numero;
	bloqueReplanificar->bloque = infoTransformacion->bloquesConSusArchivos.numBloque;
	empaquetar(socketYama, mensajeFalloTransformacion, 0 , bloqueReplanificar);
	estadisticas->cantFallos++;
	free(bloqueReplanificar);
}

void enviarJobAYama(job* miJob) {
	empaquetar(socketYama,mensajeSolicitudTransformacion, 0 ,miJob);
	log_trace(loggerMaster,"Enviando solicitud de etapa Transformacion a YAMA");

	respuesta respuestaYama = desempaquetar(socketYama);

	if(respuestaYama.idMensaje != mensajeOk){
		log_error(loggerMaster,"No se pudo iniciar correctamente el job");
		exit(1);
	}

	log_trace(loggerMaster, "Envio correcto de datos a Yama.");

}



void inicializarTiemposTransformacion(respuestaSolicitudTransformacion* infoTransformacion){
	int i,j;
	for(i=0 ; i<list_size(infoTransformacion->workers);i++){
		workerDesdeYama* worker = list_get(infoTransformacion->workers, i);
		for(j=0 ; j<list_size(worker->bloquesConSusArchivos);j++){
			bloquesConSusArchivosTransformacion* bloque = list_get(worker->bloquesConSusArchivos, j);
			setearTiempo(TRANSFORMACION,bloque->numBloque);
		}
	}
}

string* contenidoArchivo(char* pathArchivo){
	struct stat fileStat;
	if(stat(pathArchivo,&fileStat) < 0)
		exit(1);
	int fd = open(pathArchivo,O_RDWR);
	string* paquete = malloc(sizeof(string));
	paquete->cadena = mmap(0,fileStat.st_size,PROT_EXEC|PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	paquete->longitud = fileStat.st_size;

	close(fd);
	return paquete;
}



void enviarAEncargadoRedGlobal(respuestaReduccionGlobal* infoRedGlobal){
	setearTiempo(RED_GLOBAL,infoRedGlobal->numero);
	pthread_t nuevoHilo;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&nuevoHilo, &attr, (void*)conectarseConWorkerRedGlobal, infoRedGlobal) != 0) {
		log_error(loggerMaster, "No se pudo crear el thread de conexion");
		exit(-1);
	}
}

void* conectarseConWorkerRedGlobal(void* params){
	parametrosReduccionGlobal* parametrosConexion= malloc(sizeof(parametrosReduccionGlobal));
	respuestaReduccionGlobal* infoRedGlobal =(respuestaReduccionGlobal*) params;
	respuesta confirmacionWorker;

	int socketWorker = crearSocket();
	struct sockaddr_in direccion = cargarDireccion(infoRedGlobal->ip.cadena,infoRedGlobal->puerto);
	if(!conectarCon(direccion, socketWorker, 2)){//2 id master
		mandarFalloEnReduccion();
		return 0;
	}

	log_trace(loggerMaster, "Inicio Red. Global con Worker %d para Job %d", infoRedGlobal->numero, infoRedGlobal->job);

	struct stat fileStat;
	if(stat(miJob->rutaReductor.cadena,&fileStat) < 0){
		printf("No se pudo abrir el archivo\n");
		return 0;
	}

	int fd = open(miJob->rutaReductor.cadena,O_RDWR);
	int size = fileStat.st_size;

	parametrosConexion->contenidoScript.cadena = mmap(NULL,size,PROT_READ,MAP_SHARED,fd,0);
	parametrosConexion->contenidoScript.longitud = size;
	parametrosConexion->archivoTemporal.cadena = strdup(infoRedGlobal->archivoTemporal.cadena);
	parametrosConexion->archivoTemporal.longitud = infoRedGlobal->archivoTemporal.longitud;
	parametrosConexion->infoWorkers = list_create();
	list_add_all(parametrosConexion->infoWorkers,infoRedGlobal->parametros->infoWorkers);

	empaquetar(socketWorker, mensajeProcesarRedGlobal, 0, parametrosConexion);

	confirmacionWorker = desempaquetar(socketWorker);

	if (munmap(parametrosConexion->contenidoScript.cadena, parametrosConexion->contenidoScript.longitud) == -1){
		perror("Error un-mmapping the file");
		exit(EXIT_FAILURE);
	}
	close(fd);

	switch(confirmacionWorker.idMensaje){
		case mensajeOk:
		case mensajeRedGlobalCompleta:
			log_trace(loggerMaster, "Informo YAMA fin de Reduccion Global en nodo %i",infoRedGlobal->numero);
			empaquetar(socketYama, mensajeRedGlobalCompleta, 0 , 0);
			estadisticas->cantTareas[RED_GLOBAL]++;
			finalizarTiempo(estadisticas->tiempoFinRedGlobal,infoRedGlobal->numero);
			break;

		case mensajeDesconexion:
		case mensajeFalloRedGlobal:
			log_trace(loggerMaster, "Informo a  YAMA fallo en Reduccion Global del nodo %d.",infoRedGlobal->numero);
			estadisticas->cantFallos++;
			mandarFalloEnReduccion();
			break;
	}

	return 0;
}

void enviarAlmacenamientoFinal(respuestaAlmacenamiento* almacenamiento){
	pthread_t nuevoHilo;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&nuevoHilo, &attr, (void*)conectarseConWorkerAlmacenamiento, almacenamiento) != 0) {
		log_error(loggerMaster, "No se pudo crear el thread de conexion");
		exit(-1);
	}
}

void* conectarseConWorkerAlmacenamiento(void* params){
	parametrosAlmacenamiento* parametrosConexion= malloc(sizeof(parametrosAlmacenamiento));
	respuestaAlmacenamiento* almacenamiento =(respuestaAlmacenamiento*) params;
	respuesta confirmacionWorker ;

	int socketWorker = crearSocket();
	struct sockaddr_in direccion = cargarDireccion(almacenamiento->ip.cadena,almacenamiento->puerto);
	if(!conectarCon(direccion, socketWorker, 2)){//2 id master
		mandarFalloEnReduccion();
		return 0;
	}

	log_trace(loggerMaster, "Inicio Almacenamiento Final con Worker %d", almacenamiento->nodo);

	parametrosConexion->rutaAlmacenamiento.longitud = miJob->rutaResultado.longitud;
	parametrosConexion->rutaAlmacenamiento.cadena= strdup(miJob->rutaResultado.cadena);
	parametrosConexion->archivoTemporal.cadena = strdup(almacenamiento->archivo.cadena);
	parametrosConexion->archivoTemporal.longitud = almacenamiento->archivo.longitud;

	empaquetar(socketWorker, mensajeProcesarAlmacenamiento, 0, parametrosConexion);

	confirmacionWorker = desempaquetar(socketWorker);

	switch(confirmacionWorker.idMensaje){
		case mensajeAlmacenamientoCompleto:
			log_trace(loggerMaster, "Informo YAMA fin de Almacenamiento Final en nodo.",almacenamiento->nodo);
			empaquetar(socketYama, mensajeAlmacenamientoCompleto, 0 , 0);
			break;
		case mensajeFalloAlmacenamiento:
		case mensajeDesconexion:
			log_trace(loggerMaster, "Informo a YAMA fallo en Almacenamiento Final del nodo %d.",almacenamiento->nodo);
			estadisticas->cantFallos++;
			empaquetar(socketYama, mensajeFalloAlmacenamiento, 0 , 0);
			break;
	}

	return 0;
}

estadisticaProceso* crearEstadisticasProceso(){
	estadisticaProceso* estadisticas = malloc(sizeof(estadisticaProceso));
	estadisticas->tiempoInicio=time(NULL);
	estadisticas->tiempoFinTrans= list_create();
	estadisticas->tiempoFinRedLocal=list_create();
	estadisticas->tiempoFinRedGlobal= list_create();
	estadisticas->tiempoInicioTrans= list_create();
	estadisticas->tiempoInicioRedGlobal= list_create();
	estadisticas->tiempoInicioRedLocal= list_create();
	estadisticas->cantFallos=0;
	estadisticas->cantTareas[TRANSFORMACION]=0;
	estadisticas->cantTareas[RED_LOCAL]=0;
	estadisticas->cantTareas[RED_GLOBAL]=0;
	return estadisticas;
}

void setearTiempo(int etapa,int numero){
	t_list* tiemposInicio,*tiemposFin;
	switch(etapa){
		case 0:
			tiemposInicio= estadisticas->tiempoInicioTrans;
			tiemposFin=estadisticas->tiempoFinTrans;
			break;

		case 1:
			tiemposInicio= estadisticas->tiempoInicioRedLocal;
			tiemposFin=estadisticas->tiempoFinRedLocal;
			break;

		case 2:
			tiemposInicio= estadisticas->tiempoInicioRedGlobal;
			tiemposFin=estadisticas->tiempoFinRedGlobal;
			break;
	}


	tiempoPorBloque* tiempoInicio = malloc(sizeof(tiempoPorBloque));
	tiempoInicio->finalizo= true;
	tiempoInicio->numero=numero;
	tiempoInicio->tiempo=time(NULL);
	list_add(tiemposInicio,tiempoInicio);

	tiempoPorBloque* tiempoFin= malloc(sizeof(tiempoPorBloque));
	tiempoFin->finalizo= false;
	tiempoFin->numero=numero;
	tiempoFin->tiempo=time(NULL);
	list_add(tiemposFin,tiempoFin);
}

void finalizarTiempo(t_list* tiempos,int numero){
	bool encontrarEnTiempos(tiempoPorBloque* time ) {
		return time->numero == numero;
	}

	tiempoPorBloque* tiempo = list_find(tiempos,(void*)encontrarEnTiempos);
	tiempo->finalizo=true;
	tiempo->tiempo=time(NULL);
}

void finalizarJob(Finalizacion final){
	//mostrarListasEstadisticas();

	time_t fin= time(NULL);
	int duracion,cantTrans,cantLocal,cantGlobal,cantFallos;
	double promTransformacion;
	double promLocal;
	double promGlobal;


	if(final == FALLO_INGRESO){
		duracion =0;
		promGlobal=0;
		promLocal=0;
		promTransformacion=0;
		cantGlobal=0;
		cantLocal=0;
		cantTrans=0;
		cantFallos=0;
	}
	else{
		duracion = difftime(fin,estadisticas->tiempoInicio);
		promTransformacion = calcularDuracionPromedio(estadisticas->tiempoInicioTrans,estadisticas->tiempoFinTrans);
		promLocal = calcularDuracionPromedio(estadisticas->tiempoInicioRedLocal,estadisticas->tiempoFinRedLocal);
		promGlobal= calcularDuracionPromedio(estadisticas->tiempoInicioRedGlobal,estadisticas->tiempoFinRedGlobal);
		cantTrans = estadisticas->cantTareas[TRANSFORMACION];
		cantLocal = estadisticas->cantTareas[RED_LOCAL];
		cantGlobal = estadisticas->cantTareas[RED_GLOBAL];
		cantFallos = estadisticas->cantFallos;
	}

	char* finalizacion = obtenerEstadoFinalizacion(final);

	printf("\nEstado de Finalizacion: %s\n",finalizacion);
	printf("Duracion total: %d\n",duracion);
	printf("Cantidad tareas transformacion %d\n",cantTrans);
	printf("Duracion promedio transformacion: %.2f\n",promTransformacion);
	printf("Cantidad tareas reduccion local: %d\n",cantLocal);
	printf("Duracion promedio reduccion local: %.2f\n",promLocal);
	printf("Cantidad tareas reduccion global: %d\n",cantGlobal);
	printf("Duracion promedio reduccion global: %.2f\n",promGlobal);
	printf("Cantidad de fallos obtenidos: %d\n",cantFallos);

	exit(0);
}

double calcularDuracionPromedio(t_list* tiemposInicio,t_list* tiemposFin){
	int i;
	double cont=0;

	if(list_is_empty(tiemposFin)){
		return 0;
	}

	for(i=0;i<list_size(tiemposInicio);i++){
		tiempoPorBloque* inicio = list_get(tiemposInicio,i);
		tiempoPorBloque* fin = list_get(tiemposFin,i);
		if(fin->finalizo){
			cont = cont + difftime(fin->tiempo,inicio->tiempo);
		}

	}

	if(cont==0){
		return 0.1;
	}

	return cont / list_size(tiemposInicio);

}

void mostrarListasEstadisticas(){
	void mostrar(tiempoPorBloque* tiempo){
		printf("NUMERO: %d || FINALIZO: %d || TIEMPO: %s \n",tiempo->numero,tiempo->finalizo,ctime(&tiempo->tiempo));
	}


	printf("\n inicio transformacion \n");
	list_iterate(estadisticas->tiempoInicioTrans,(void*)mostrar);

	printf("\n fin transformacion \n");
	list_iterate(estadisticas->tiempoFinTrans,(void*)mostrar);

	//list_iterate(estadisticas->tiempoInicioRedLocal,(void*)mostrar);
	//list_iterate(estadisticas->tiempoFinRedLocal,(void*)mostrar);
	//list_iterate(estadisticas->tiempoInicioRedGlobal,(void*)mostrar);
	//list_iterate(estadisticas->tiempoFinRedGlobal,(void*)mostrar);
}

char* obtenerEstadoFinalizacion(Finalizacion tipoFin){
	char* nombre;

	switch(tipoFin){
	case OK:
		nombre =malloc(3);
		nombre = strdup("OK");
		break;

	case FALLO_ALMACENAMIENTO:
		nombre =malloc(21);
		nombre = strdup("FALLO ALMACENAMIENTO");
		break;

	case FALLO_RED_GLOBAL:
		nombre =malloc(16);
		nombre = strdup("FALLO RED GLOBAL");
		break;

	case FALLO_RED_LOCAL:
		nombre =malloc(15);
		nombre = strdup("FALLO RED LOCAL");
		break;

	case FALLO_TRANSFORMACION:
		nombre =malloc(23);
		nombre = strdup("FALLO TRANSFORMACION");
		break;

	case FALLO_INGRESO:
		nombre =malloc(14);
		nombre = strdup("FALLO INGRESO");
		break;
	}

	return nombre;
}
