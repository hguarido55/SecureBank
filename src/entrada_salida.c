#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "utils.h"

BufferEstructurado buffer = 
{
    .inicio = 0,
    .fin = 0
};

pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_buffer = PTHREAD_COND_INITIALIZER;

void buffer_push(Cuenta op) 
{
    pthread_mutex_lock(&mutex_buffer);

    int next = (buffer.fin + 1) % BUFFER_MAX;

    // Si el buffer está lleno, espera a que haya espacio
    while (next == buffer.inicio) 
    {
        // Esperamos a que el hilo consumidor libere espacio
        pthread_cond_wait(&cond_buffer, &mutex_buffer);
        next = (buffer.fin + 1) % BUFFER_MAX;
    }

    buffer.operaciones[buffer.fin] = op;
    buffer.fin = next;

    pthread_cond_signal(&cond_buffer); // Notificamos que hay datos
    pthread_mutex_unlock(&mutex_buffer);
}

// Un hilo dedicado gestiona las escrituras desde el buffer al disco
void *gestionar_entrada_salida(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex_buffer);
        while (buffer.inicio == buffer.fin) {
            // Buffer vacío, espera hasta que haya datos
            pthread_cond_wait(&cond_buffer, &mutex_buffer);
        }

        Cuenta op = buffer.operaciones[buffer.inicio];
        buffer.inicio = (buffer.inicio + 1) % BUFFER_MAX;

        pthread_cond_signal(&cond_buffer); // Notifica que hay espacio
        pthread_mutex_unlock(&mutex_buffer);

        FILE *archivo = fopen("cuentas.dat", "rb+");
        if (!archivo) {
            perror("Error abriendo cuentas.dat");
            continue;
        }

        long pos = (op.numero_cuenta - 1) * sizeof(Cuenta);
        if (fseek(archivo, pos, SEEK_SET) != 0) {
            perror("Error en fseek");
            fclose(archivo);
            continue;
        }

        if (fwrite(&op, sizeof(Cuenta), 1, archivo) != 1) {
            perror("Error escribiendo en cuentas.dat");
        }
        fclose(archivo);
    }
    return NULL;
}

void sincronizar_cuenta_desde_disco(int numeroCuenta, TablaCuentas *tabla)
{
    FILE *archivo = fopen("../data/cuentas.txt", "r");
    if (!archivo) 
    {
        perror("Error al abrir cuentas.txt para sincronización");
        return;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) 
    {
        int num;
        char titular[50];
        float saldo;
        int transacciones;

        if (sscanf(linea, "%d,%49[^,],%f,%d", &num, titular, &saldo, &transacciones) == 4) {
            if (num == numeroCuenta) {
                for (int i = 0; i < tabla->num_cuentas; i++) {
                    if (tabla->cuentas[i].numero_cuenta == numeroCuenta) {
                        tabla->cuentas[i].saldo = saldo;
                        tabla->cuentas[i].num_transacciones = transacciones;
                        strncpy(tabla->cuentas[i].titular, titular, sizeof(tabla->cuentas[i].titular));
                        tabla->cuentas[i].titular[sizeof(tabla->cuentas[i].titular) - 1] = '\0';
                        break;
                    }
                }
                break; // Sincronizamos
            }
        }
    }
    fclose(archivo);
}