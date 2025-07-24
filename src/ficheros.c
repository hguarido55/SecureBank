#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "utils.h"

void crear_carpetas_transacciones() 
{
    int i;
    for(i = 0; i < MAX_USUARIOS; i++) 
    {
        printf("Creando carpeta de transacciones para el usuario %d\n", CuentasIniciales[i].numero_cuenta);

        char path[100];
        snprintf(path, sizeof(path), "../data/transacciones/%d", CuentasIniciales[i].numero_cuenta);

        if (mkdir(path, 0777) == -1) 
        {
            if (errno != EEXIST) 
            { 
                // Si no es un error de existencia de la carpeta
                perror("Error al crear la carpeta de transacciones para el usuario");
                return;
            }
        }

        // Crear el archivo transacciones.log dentro de la carpeta del usuario
        char archivo_transacciones[150];
        snprintf(archivo_transacciones, sizeof(archivo_transacciones), "../data/transacciones/%d/transacciones.log", CuentasIniciales[i].numero_cuenta);
        
        FILE *transacciones_log = fopen(archivo_transacciones, "w");
        if (transacciones_log == NULL) 
        {
            perror("Error al crear el archivo transacciones.log");
            return;
        }

        // Escribir un mensaje inicial en el archivo de transacciones para cada usuario
        fprintf(transacciones_log, "Registro de transacciones para el usuario %d (%s)\n", 
                CuentasIniciales[i].numero_cuenta, CuentasIniciales[i].titular);
        fclose(transacciones_log);
        
    }
    printf("Se han inicializado las cuentas y los archivos de transacciones correctamente.\n");
}

// Funcion para registro de operacion en el log del usuario
void RegistrarOperaciones(int numeroCuenta, float cantidad, const char *operacion) 
{
    FILE *RegistroTransacciones;
    char fechaActual[32];
    char archivoUsuario[256];
    time_t t;
    struct tm *tiempo;

    t = time(NULL);
    tiempo = localtime(&t);

    sprintf(fechaActual, "%04d-%02d-%02d %02d:%02d:%02d",
        tiempo->tm_year + 1900, tiempo->tm_mon + 1, tiempo->tm_mday,
        tiempo->tm_hour, tiempo->tm_min, tiempo->tm_sec);

    snprintf(archivoUsuario, sizeof(archivoUsuario), "../data/transacciones/%d/transacciones.log", numeroCuenta);
    RegistroTransacciones = fopen(archivoUsuario,"a");

    if(RegistroTransacciones == NULL) 
    {
        perror("Error al abrir el archivo de transacciones de la cuenta.");
        return;
    }

    fprintf(RegistroTransacciones,"[%s] %s en cuenta %d: %.2f\n", fechaActual, operacion, numeroCuenta, cantidad);
    fclose(RegistroTransacciones);
}