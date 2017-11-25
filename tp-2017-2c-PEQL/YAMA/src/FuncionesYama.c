/*
 * FuncionesYama.c
 *
 *  Created on: 24/9/2017
 *      Author: utnso
 */
#include "FuncionesYama.h"
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/inotify.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )
#define BUF_LEN     ( 1024 * EVENT_SIZE )

int disponibilidadBase;

int conectarseConFs() {
	respuesta respuesta;
	int socketFs = crearSocket();
	struct sockaddr_in direccion = cargarDireccion(config.FS_IP, config.FS_PUERTO);
	conectarCon(direccion, socketFs, 1);
	respuesta = desempaquetar(socketFs);
	if (respuesta.idMensaje == mensajeNoEstable){
		printf("File system en estado inestable, no se puede conectar\n");
		exit(1);
	}

	log_trace(logger, "Conexion exitosa con File System");
	return socketFs;
}

void *manejarConexionMaster(void *cliente) {
	int nuevoMaster;
	memcpy(&nuevoMaster,cliente,sizeof(int));

	recibirContenidoMaster(nuevoMaster);
	return 0;
}

void manejarConfig(){
	while (1) {
		verCambiosConfig();
	}
}

void levantarServidorYama(char* ip, int port){
	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion = sizeof(direccionCliente);
	respuesta respuestaId;
	void* cliente;

	servidor = crearServidorAsociado(ip, port);

	while (1) {
		int nuevoCliente;
		if ((nuevoCliente = accept(servidor,(struct sockaddr *) &direccionCliente, &tamanioDireccion))!= -1) {

			pthread_t nuevoHilo;
			pthread_attr_t attr;

			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			respuestaId = desempaquetar(nuevoCliente);
			int id = *(int*)respuestaId.envio;

			if(id == idMaster){
				pthread_mutex_lock(&mutexLog);
				log_trace(logger, "Nueva conexion de un Master\n");
				pthread_mutex_unlock(&mutexLog);

				empaquetar(nuevoCliente,mensajeOk,0,0);

				cliente= malloc(sizeof(int));
				memcpy(cliente,&nuevoCliente,sizeof(int));

				if (pthread_create(&nuevoHilo, &attr, &manejarConexionMaster,cliente) == -1) {
					log_error(logger, "could not create thread");
					perror("could not create thread");
					log_destroy(logger);
					exit(1);
				}
			}
			else{
				pthread_mutex_lock(&mutexLog);
				log_trace(logger, "Conexion invalida\n");
				pthread_mutex_unlock(&mutexLog);
			}
		}
	}
}

void verCambiosConfig(){
	char *pathCarpetaConfig = string_new();
	char  cwdCarpeta[1024];

	string_append(&pathCarpetaConfig, getcwd(cwdCarpeta, sizeof(cwdCarpeta)));

	char buffer[BUF_LEN];

	int file_descriptor = inotify_init();

	if (file_descriptor < 0) {
		log_error(logger, "<inotify> No se pudo iniciar.");
	}

	int watch_descriptor = inotify_add_watch(file_descriptor, pathCarpetaConfig, IN_MODIFY | IN_DELETE);

	int length = read(file_descriptor, buffer, BUF_LEN);

	if (length < 0) {
		log_error(logger, "<inotify> No se pudo leer archivo de configuracion.");
	}

	int offset = 0;

	while (offset < length) {
		struct inotify_event *event = (struct inotify_event *)&buffer[offset];

		if (event->len) {
			if (strcmp(event->name, "YAMA.cfg") == 0) {
				if (event->mask & IN_DELETE) {
					if (event->mask & IN_ISDIR) {
						log_error(logger, "<inotify> El directorio %s ha sido eliminado.", event->name);
					}
					else {
						log_error(logger, "<inotify> El archivo %s ha sido eliminado.", event->name);
					}
				}
				else if (event->mask & IN_MODIFY) {
					if (event->mask & IN_ISDIR) {
						log_info(logger, "<inotify> El directorio %s ha sido modificado.", event->name);
					}
					else {
						log_info(logger, "<inotify> El archivo %s ha sido modificado.", event->name);
						validarCambiosConfig();
					}
	            }
	         }
	      }
	      offset += sizeof(struct inotify_event) + event->len;
	   }

	   inotify_rm_watch(file_descriptor, watch_descriptor);
	   close(file_descriptor);

}

void validarCambiosConfig(){
	int nuevoRetardo,nuevaDisponibilidad;
	char* nuevoAlgoritmo;

	char *pathArchConfig = string_new(); // String que va a tener el path absoluto para pasarle al config_create

	char cwd[1024];                      // Variable donde voy a guardar el path absoluto hasta el /Debug

	string_append(&pathArchConfig, getcwd(cwd, sizeof(cwd)));

	string_append(&pathArchConfig, "/YAMA.cfg");     // Le concateno el archivo de configuración

	t_config *archivoConfig = config_create(pathArchConfig);

	if (archivoConfig == NULL) {
		log_info(logger, "<inotify> config_create retorno NULL.");
		return;
	}

	if (config_has_property(archivoConfig, "RETARDO_PLANIFICACION")) {
		nuevoRetardo= config_get_int_value(archivoConfig, "RETARDO_PLANIFICACION");
	}
	else{
		log_info(logger, "<inotify> RETARDO PLANIFICACION retorno NULL.");
		return;
	}

	if (config_has_property(archivoConfig, "DISPONIBILIDAD_BASE")) {
		nuevaDisponibilidad= config_get_int_value(archivoConfig, "DISPONIBILIDAD_BASE");
	}
	else{
		log_info(logger, "<inotify> DISPONIBILIDAD_BASE retorno NULL.");
		return;
	}

	if (config_has_property(archivoConfig, "ALGORITMO_BALANCEO")) {
		nuevoAlgoritmo= config_get_string_value(archivoConfig, "ALGORITMO_BALANCEO");
	}
	else{
		log_info(logger, "<inotify> ALGORITMO_BALANCEO retorno NULL.");
		return;
	}

	//config_destroy(archivoConfig);

	if (config.RETARDO_PLANIFICACION != nuevoRetardo) {
		log_info(logger, "<inotify> RETARDO_PLANIFICACION modificado. Anterior: %d || Actual: %d", config.RETARDO_PLANIFICACION , nuevoRetardo);

		if (nuevoRetardo<= 0) {
			log_error(logger, "El RETARDO_PLANIFICACION no puede ser < = 0. Se deja el anterior: %d.", config.RETARDO_PLANIFICACION);
		}
		else{
			log_trace(logger,"[Inotify] RETARDO_PLANIFICACION modificado. Anterior: %d || Actual: %d\n", config.RETARDO_PLANIFICACION, nuevoRetardo);
			config.RETARDO_PLANIFICACION = nuevoRetardo;
		}
	}

	if (config.DISPONIBILIDAD_BASE != nuevaDisponibilidad) {
		log_info(logger, "<inotify> DISPONIBILIDAD_BASE modificada. Anterior: %d || Actual: %d", config.DISPONIBILIDAD_BASE , nuevaDisponibilidad);

		if (nuevoRetardo<= 0) {
			log_error(logger, "La DISPONIBILIDAD_BASE no puede ser < = 0. Se deja la anterior: %d.", config.DISPONIBILIDAD_BASE);
		}
		else{
			log_trace(logger,"[Inotify] DISPONIBILIDAD_BASE modificada. Anterior: %d || Actual: %d\n", config.DISPONIBILIDAD_BASE, nuevaDisponibilidad);
			config.DISPONIBILIDAD_BASE = nuevoRetardo;
		}
	}

	if (strcmp(config.ALGORITMO_BALANCEO ,nuevoAlgoritmo)) {
		log_info(logger, "<inotify> ALGORITMO_BALANCEO modificada. Anterior: %s || Actual: %s", config.ALGORITMO_BALANCEO , nuevoAlgoritmo);

		if (!string_equals_ignore_case(nuevoAlgoritmo,"WCLOCK") && !string_equals_ignore_case(nuevoAlgoritmo,"CLOCK")) {
			log_error(logger, "El ALGORITMO_BALANCEO no puede ser diferente a CLOCK o WCLOCK. Se deja el anterior: %s.", config.ALGORITMO_BALANCEO);
		}
		else{
			log_trace(logger,"[Inotify] ALGORITMO_BALANCEO modificada. Anterior: %s || Actual: %s\n", config.ALGORITMO_BALANCEO, nuevoAlgoritmo);
			config.ALGORITMO_BALANCEO = strdup(nuevoAlgoritmo);
		}
	}
	config_destroy(archivoConfig);
}

void recibirContenidoMaster(int nuevoMaster) {
	respuesta nuevoJob;

	iniciarListasPlanificacion();


	nuevoJob = desempaquetar(nuevoMaster);

	if(nuevoJob.idMensaje == mensajeDesconexion){
		log_trace(logger, "Fallo al recibir job");
		pthread_exit(0);
	}

	job* jobAPlanificar =(job*) nuevoJob.envio;

	pthread_mutex_lock(&cantJobs_mutex);
	cantJobs++;
	jobAPlanificar->id = cantJobs;
	jobAPlanificar->socketFd = nuevoMaster;
	pthread_mutex_unlock(&cantJobs_mutex);
	log_trace(logger, "Job recibido para pre-planificacion %i",jobAPlanificar->id);

	empaquetar(nuevoMaster, mensajeOk, 0,0);

	planificar(jobAPlanificar);

}

informacionArchivoFsYama* solicitarInformacionAFS(solicitudInfoNodos* solicitud){
	informacionArchivoFsYama* rtaFs = malloc(sizeof(informacionArchivoFsYama));
	respuesta respuestaFs;

	empaquetar(socketFs, mensajeSolicitudInfoNodos, 0, solicitud);

	respuestaFs = desempaquetar(socketFs);

	if(respuestaFs.idMensaje == mensajeRespuestaInfoNodos){
		rtaFs = (informacionArchivoFsYama*)respuestaFs.envio;
		log_trace(logger, "Me llego la informacion desde Fs correctamente");
	}
	else{
		log_error(logger, "Error al recibir la informacion del archivo desde FS");
		exit(1);
	}
	return rtaFs;
}

int getDisponibilidadBase(){
	return config.DISPONIBILIDAD_BASE;
}

int esClock(){
	return strcmp("CLOCK" ,config.ALGORITMO_BALANCEO);
}

void recibirArchivo(int nuevoMaster){
	respuesta paquete;

	paquete = desempaquetar(nuevoMaster);
	string* archivo = (string*) paquete.envio;
	char* hola = archivo->cadena;

	printf("%s\n", hola);
}

char* dameUnNombreArchivoTemporal(int jobId,int numBloque,int etapa,int nodo){
	char* nombre= string_from_format("j%dn%db%de%d",jobId, nodo,numBloque,etapa);
	return nombre;
}
void inicializarEstructuras(){
	pthread_mutex_init(&cantJobs_mutex, NULL);
	pthread_mutex_init(&mutexTablaEstados, NULL);
	pthread_mutex_init(&mutexLog, NULL);
	pthread_mutex_init(&mutexConfiguracion, NULL);
	pthread_mutex_init(&mutexJobs, NULL);
}

bool** llenarMatrizNodosBloques(informacionArchivoFsYama* infoArchivo,int nodos,int bloques){
	bool** matriz = (bool**)malloc((nodos+1)*sizeof(bool*));

	int j,k;
	for(j=0;j<=nodos;j++){
		matriz[j]= malloc(bloques * sizeof(bool));
		for(k=0;k<bloques;k++){
			matriz[j][k] = false;
		}
	}

	int i;
	for(i=0;i< list_size(infoArchivo->informacionBloques);i++){
		infoBloque* info = list_get(infoArchivo->informacionBloques,i);
		matriz[info->ubicacionCopia0.numeroNodo][info->numeroBloque] = true;
		matriz[info->ubicacionCopia1.numeroNodo][info->numeroBloque] = true;
	}

	return matriz;
}

int nodoConOtraCopia(bloqueYNodo* replanificar,bool** matriz,int nodos,int bloques){
	int i=0;

	for(i=0;i<=bloques;i++){
		matriz[replanificar->workerId][i]=false;
	}

	for(i=0;i<=nodos;i++){
		if(matriz[i][replanificar->bloque] && (i != replanificar->workerId)){
			return i;
		}
	}
	return -1;
}

void calcularNodosYBloques(informacionArchivoFsYama* info,int* nodos,int*bloques){
	*bloques = list_size(info->informacionBloques);

	int max =0;
	int i;
	for(i=0;i<list_size(info->informacionBloques);i++){
		infoBloque* infoBlo = list_get(info->informacionBloques,i);
		if(infoBlo->ubicacionCopia0.numeroNodo > max){
			max = infoBlo->ubicacionCopia0.numeroNodo;
		}
		if(infoBlo->ubicacionCopia1.numeroNodo > max){
			max = infoBlo->ubicacionCopia1.numeroNodo;
		}
	}
	*nodos = max;
}

void llenarListaNodos(t_list* listaNodos,informacionArchivoFsYama* infoArchivo){
	int i;
	for(i=0;i<list_size(infoArchivo->informacionBloques);i++){
		infoBloque* infoBlo = list_get(infoArchivo->informacionBloques,i);
		agregarBloqueANodo(listaNodos,infoBlo->ubicacionCopia0,infoBlo->numeroBloque);
		agregarBloqueANodo(listaNodos,infoBlo->ubicacionCopia1,infoBlo->numeroBloque);
	}
}

void agregarBloqueANodo(t_list* listaNodos,ubicacionBloque ubicacion,int bloque){
	infoNodo* nodoAPlanificar = malloc(sizeof(infoNodo));
	nodoAPlanificar->bloques = list_create();

	bool funcionFind(void *nodo) {
		return(((infoNodo*)nodo)->numero == ubicacion.numeroNodo);
	}

	pthread_mutex_lock(&mutex_NodosConectados);

	if(list_find(listaNodos,funcionFind)){
		infoNodo* nodo = (infoNodo*)list_find(listaNodos,funcionFind);
		list_add(nodo->bloques,&bloque);
	}
	else if (list_find(nodosConectados,funcionFind)) {
		infoNodo* nodo = (infoNodo*)list_find(nodosConectados,funcionFind);
		memcpy(nodoAPlanificar,nodo,sizeof(infoNodo));
		nodoAPlanificar->bloques = list_create();
		list_add(nodoAPlanificar->bloques,&bloque);
		list_add(listaNodos,nodoAPlanificar);
	}
	else{
		infoNodo* nuevoNodo = malloc(sizeof(infoNodo));
		nuevoNodo->carga=0;
		nuevoNodo->cantTareasHistoricas=0;
		nuevoNodo->conectado = true;
		nuevoNodo->ip.cadena = strdup(ubicacion.ip.cadena);
		nuevoNodo->ip.longitud = ubicacion.ip.longitud;
		nuevoNodo->numero = ubicacion.numeroNodo;
		nuevoNodo->puerto = ubicacion.puerto;
		list_add(nodosConectados,nuevoNodo);

		memcpy(nodoAPlanificar,nuevoNodo,sizeof(infoNodo));
		nodoAPlanificar->bloques = list_create();
		list_add(nodoAPlanificar->bloques,&bloque);
		list_add(listaNodos,nodoAPlanificar);

	}
	pthread_mutex_unlock(&mutex_NodosConectados);

}

void agregarBloqueTerminadoATablaEstados(int bloque,int jobId,Etapa etapa){
	bool encontrarEnTablaEstados(registroTablaEstados* reg ) {
		return reg->job == jobId && reg->etapa == etapa && reg->estado == EN_EJECUCION && reg->bloque==bloque;
	}

	pthread_mutex_lock(&mutexTablaEstados);
	registroTablaEstados* reg = list_find(tablaDeEstados,(void*)encontrarEnTablaEstados);
	reg->estado=FINALIZADO_OK;
	pthread_mutex_unlock(&mutexTablaEstados);
}
void agregarBloqueTerminadoATablaEstadosRedLocal(int nodo,int jobId,Etapa etapa){
	bool encontrarEnTablaEstados(registroTablaEstados* reg ) {
		return reg->job == jobId && reg->etapa == etapa && reg->estado == EN_EJECUCION && reg->nodo==nodo;
	}
	void modificarEstado(registroTablaEstados* reg){
		if(encontrarEnTablaEstados(reg)){
			reg->estado=FINALIZADO_OK;
		}
	}
	pthread_mutex_lock(&mutexTablaEstados);
	list_iterate(tablaDeEstados, (void*)modificarEstado);
	pthread_mutex_unlock(&mutexTablaEstados);


}
bool faltanMasTareas(int jobid,Etapa etapa){
	bool encontrarEnTablaEstados(registroTablaEstados* reg) {
		return reg->job == jobid && reg->etapa == etapa && reg->estado == EN_EJECUCION;
	}

	pthread_mutex_lock(&mutexTablaEstados);
	bool faltanTareas = list_any_satisfy(tablaDeEstados,(void*)encontrarEnTablaEstados);
	pthread_mutex_unlock(&mutexTablaEstados);

	return faltanTareas;
}

void finalizarJob(job* job,int etapa,int error){
	//actualizarCargasNodos(job->id,etapa);
	reducirCargaJob(job);
	borrarEntradasDeJob(job->id);
	int err = error;
	empaquetar(job->socketFd,mensajeFinJob,0,&err);
	respuesta respuestaFin;
	respuestaFin = desempaquetar(job->socketFd);

	while(respuestaFin.idMensaje != mensajeDesconexion){
		respuestaFin = desempaquetar(job->socketFd);
	}

	log_trace(logger, "Finalizo job con id: %d",job->id);
	close(job->socketFd);
	free(job);
	pthread_exit(0);
}
void reducirCargaJob(job* unJob){
	bool encontrarEnTablaEstados(registroTablaEstados* reg) {
		return reg->job == unJob->id && reg->estado==FINALIZADO_OK;
	}

	void actualizarCarga(registroTablaEstados* reg){
		if(encontrarEnTablaEstados(reg)){
			infoNodo* nodo = obtenerNodo(reg->nodo);
			if(reg->nodo == nodo->numero){
				nodo->carga -= 1;
			}
		}
	}
	pthread_mutex_lock(&mutexTablaEstados);
	list_iterate(tablaDeEstados, (void*)actualizarCarga);
	pthread_mutex_unlock(&mutexTablaEstados);
}
void actualizarCargasNodos(int jobid,Etapa etapa){
	bool encontrarEnTablaEstados(registroTablaEstados* reg) {
		return reg->job == jobid && reg->etapa == etapa;
	}

	void actualizarCarga(registroTablaEstados* reg ){
		if(encontrarEnTablaEstados(reg)){
			infoNodo* nodo = obtenerNodo(reg->nodo);
			nodo->carga++;
			nodo->cantTareasHistoricas++;
		}
	}

	pthread_mutex_lock(&mutex_NodosConectados);
	list_iterate(tablaDeEstados,(void*)actualizarCarga);
	pthread_mutex_unlock(&mutex_NodosConectados);
}

void actualizarCargasNodosRedLocal(int jobid,int numNodo){
	bool encontrarEnTablaEstados(registroTablaEstados* reg) {
		return reg->job == jobid && reg->etapa == RED_LOCAL && reg->nodo==numNodo;
	}
	void actualizarCarga(registroTablaEstados* reg ){
		if(encontrarEnTablaEstados(reg)){
			infoNodo* nodo = obtenerNodo(reg->nodo);
			nodo->carga++;
			nodo->cantTareasHistoricas++;
		}
	}
	pthread_mutex_lock(&mutexTablaEstados);
	list_iterate(tablaDeEstados,(void*)actualizarCarga);
	pthread_mutex_unlock(&mutexTablaEstados);

}

void actualizarCargasNodosReplanificacion(int jobid,int etapa,int bloque){
	bool encontrarEnTablaEstados(void *registro) {
		registroTablaEstados* reg =(registroTablaEstados*)registro;
		return reg->job == jobid && reg->etapa== etapa && reg->bloque==bloque;
	}

	pthread_mutex_lock(&mutexTablaEstados);
	registroTablaEstados* reg = list_find(tablaDeEstados,(void*)encontrarEnTablaEstados);
	pthread_mutex_unlock(&mutexTablaEstados);

	pthread_mutex_lock(&mutex_NodosConectados);
	infoNodo* nodo = obtenerNodo(reg->nodo);
	nodo->carga = nodo->carga-1;
	pthread_mutex_unlock(&mutex_NodosConectados);

}

void borrarEntradasDeJob(int jobid){
	bool encontrarEnTablaEstados(void *registro) {
		return(((registroTablaEstados*)registro)->job== jobid);
	}

	pthread_mutex_lock(&mutexTablaEstados);
	while(list_find(tablaDeEstados,(void*)encontrarEnTablaEstados)){
		list_remove_by_condition(tablaDeEstados,(void*)encontrarEnTablaEstados);
	}
	pthread_mutex_unlock(&mutexTablaEstados);

}

void esperarRespuestaReduccionDeMaster(job* job){
	respuesta respuestaMaster=  desempaquetar(job->socketFd);
	if(respuestaMaster.idMensaje == mensajeRedGlobalCompleta){
		agregarBloqueTerminadoATablaEstados(0,job->id,RED_GLOBAL);
		return;
	}
	else if(respuestaMaster.idMensaje == mensajeFalloRedGlobal){
		finalizarJob(job,RED_GLOBAL,FALLO_RED_GLOBAL);
	}
	else if(respuestaMaster.idMensaje == mensajeDesconexion){
		log_error(logger, "Error en Proceso Master.");
		reestablecerEstadoYama(job);
	}
}
void reestablecerEstadoYama(job* job){
	close(job->socketFd);
	free(job);
	pthread_exit(0);
}
infoNodo* obtenerNodo(int numero){
	bool encontrarNodo(void *nodo){
		return ((infoNodo*)nodo)->numero == numero;
	}

	return list_find(nodosConectados,(void*)encontrarNodo);
}

void eliminarWorker(int id, t_list* listaNodos){
	bool nodoConNumero(infoNodo* worker){
		return worker->numero == id;
	}
	infoNodo* workerEnGlobal = malloc(sizeof(infoNodo));
	infoNodo* worker = malloc(sizeof(infoNodo));

	pthread_mutex_lock(&mutex_NodosConectados);
	workerEnGlobal = list_find(nodosConectados, (void*)nodoConNumero);
	workerEnGlobal->conectado = false;
	pthread_mutex_unlock(&mutex_NodosConectados);

	worker = list_find(listaNodos, (void*)nodoConNumero);
	worker->conectado = false;

}

bool faltanBloquesTransformacionParaNodo(int jobid,int nodo){
	bool encontrarEnTablaEstados(registroTablaEstados* reg) {
		return reg->job == jobid && reg->etapa == TRANSFORMACION && reg->estado == EN_EJECUCION && reg->nodo==nodo;
	}

	pthread_mutex_lock(&mutexTablaEstados);
	bool faltanTareas = list_any_satisfy(tablaDeEstados,(void*)encontrarEnTablaEstados);
	pthread_mutex_unlock(&mutexTablaEstados);

	return faltanTareas;
}

void mostrarTablaDeEstados(){
	void iterar(registroTablaEstados* reg){
		printf("numero %d, bloque %d, etapa %d estado %d, ruta %s \n\n",reg->nodo,reg->bloque,reg->etapa,reg->estado,reg->rutaArchivoTemp);
	}
	pthread_mutex_lock(&mutexTablaEstados);
	list_iterate(tablaDeEstados,(void*)iterar);
	pthread_mutex_unlock(&mutexTablaEstados);
}
