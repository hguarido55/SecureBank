#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <errno.h>
#include "utils.h"

int shm_id = -1;
TablaCuentas *tabla = NULL;

TablaCuentas *crear_memoria_compartida() {
    shm_id = shmget(IPC_PRIVATE, sizeof(TablaCuentas), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error al crear memoria compartida");
        exit(1);
    }

    tabla = (TablaCuentas *)shmat(shm_id, NULL, 0);
    if (tabla == (void *)-1) {
        perror("Error al adjuntar memoria compartida");
        exit(1);
    }

    return tabla;
}

void liberar_memoria_compartida() 
{
    if (shmdt(tabla) == -1) 
    {
        perror("Error al desasociar memoria compartida");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) 
    {
        perror("Error al eliminar memoria compartida");
    }
}

int obtener_shm_id() 
{
    return shm_id;
}

TablaCuentas *obtener_tabla() 
{
    return tabla;
}