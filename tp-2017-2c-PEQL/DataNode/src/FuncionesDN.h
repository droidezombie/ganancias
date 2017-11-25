#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

void conectarseConFs();

int levantarBitmap(char* nombreNodo);

void escucharAlFS(int socketFs);

int setBloque(int numeroBloque, char* datos);

char* getBloque(int numeroBloque);

void inicializarDataBin();

void recibirMensajesFileSystem(int socketFs);
