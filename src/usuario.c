#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include "utils.h"

#define ARCHIVO_CUENTAS "../data/cuentas.dat"
#define LIMITE_DEPOSITO 10000.0
#define LIMITE_TRANSFERENCIA 10000.0
#define CuentasTXT "../data/cuentas.txt"
#define CONFIG_PATH "../data/config.txt"

sem_t *sem_cuentas;
pthread_mutex_t mutex_operaciones;
pthread_mutex_t mutex_tabla_local = PTHREAD_MUTEX_INITIALIZER;
Config config;
int write_fd;

void RealizarConsulta(int NumCuenta);
Config leer_configuracion(const char *ruta);

Config leer_configuracion(const char *ruta) {
    Config config;
    char linea[100];
    FILE *archivo = fopen(ruta, "r");

    if (archivo == NULL) {
        perror("Error al abrir config.txt");
        exit(1);
    }

    while (fgets(linea, sizeof(linea), archivo)) {
        if (linea[0] == '#' || strlen(linea) < 3) continue;
        if (strstr(linea, "LIMITE_RETIRO")) sscanf(linea, "LIMITE_RETIRO=%d", &config.limite_retiro);
        else if (strstr(linea, "LIMITE_TRANSFERENCIA")) sscanf(linea,"LIMITE_TRANSFERENCIA=%d", &config.limite_transferencia);
        else if (strstr(linea, "UMBRAL_RETIROS")) sscanf(linea, "UMBRAL_RETIROS=%d",&config.umbral_retiros);
        else if (strstr(linea, "UMBRAL_TRANSFERENCIAS")) sscanf(linea,"UMBRAL_TRANSFERENCIAS=%d", &config.umbral_transferencias);
        else if (strstr(linea, "NUM_HILOS")) sscanf(linea, "NUM_HILOS=%d",&config.num_hilos);
        else if (strstr(linea, "ARCHIVO_CUENTAS")) sscanf(linea, "ARCHIVO_CUENTAS=%s",config.archivo_cuentas);
        else if (strstr(linea, "ARCHIVO_LOG")) sscanf(linea, "ARCHIVO_LOG=%s",config.archivo_log);
    }
    fclose(archivo);
    return config;
}

void RealizarConsulta(int NumCuenta) 
{
    sem_wait(sem_cuentas);

    sincronizar_cuenta_desde_disco(NumCuenta, tabla);

    int encontrado = 0;

    for (int i = 0; i < tabla->num_cuentas; i++) 
    {
        if (tabla->cuentas[i].numero_cuenta == NumCuenta) 
        {
            printf("Cuenta: %d\nTitular: %s\nSaldo: %.2f\n",
                tabla->cuentas[i].numero_cuenta,
                tabla->cuentas[i].titular,
                tabla->cuentas[i].saldo);
            encontrado = 1;
            break;
        }
    }

    if (!encontrado)
    {
        printf("Cuenta no encontrada.\n");
    }

    sem_post(sem_cuentas);
}

float RealizarDeposito(int NumCuenta) 
{
    sem_wait(sem_cuentas);

    float deposito;
    int encontrado = 0;
    long pos;

    printf("Ingrese cantidad a depositar: ");
    scanf("%f", &deposito);

    if (deposito > 3 * LIMITE_DEPOSITO) 
    {
        printf("Depósito excede el límite permitido. Operación cancelada.\n");
        char mensaje[256];
        snprintf(mensaje, sizeof(mensaje), "CUENTA:%d|OPERACION:Deposito ANÓMALO|CANTIDAD:%.2f", NumCuenta, deposito);
        write(write_fd, mensaje, strlen(mensaje));
        sem_post(sem_cuentas);
        return 0;
    }
    
    for (int i = 0; i < tabla->num_cuentas; i++) 
    {
        if (tabla->cuentas[i].numero_cuenta == NumCuenta) 
        {
            tabla->cuentas[i].saldo += deposito;
            tabla->cuentas[i].num_transacciones++;
            buffer_push(tabla->cuentas[i]);
            encontrado = 1;
            printf("Depósito exitoso. Nuevo saldo: %.2f\n", tabla->cuentas[i].saldo);
            break;
        }
    }

    if (!encontrado) 
    {
        printf("Cuenta no encontrada.\n");
        deposito = 0;
    }

    sem_post(sem_cuentas);
    return deposito;
}

float RealizarRetirada(int NumCuenta) 
{
    sem_wait(sem_cuentas);

    float retirada;
    int encontrado = 0;

    printf("Ingrese cantidad a retirar: ");
    scanf("%f", &retirada);

    if (retirada > config.limite_retiro) 
    {
        printf("Retirada excede el límite permitido. Operación cancelada.\n");
        char mensaje[256];
        snprintf(mensaje, sizeof(mensaje), "CUENTA:%d|OPERACION:Retirada ANÓMALA|CANTIDAD:%.2f", NumCuenta, retirada);
        write(write_fd, mensaje, strlen(mensaje));
        sem_post(sem_cuentas);
        return 0;
    }

     for (int i = 0; i < tabla->num_cuentas; i++) 
     {
        if (tabla->cuentas[i].numero_cuenta == NumCuenta) 
        {
            encontrado = 1;
            if (tabla->cuentas[i].saldo >= retirada) 
            {
                tabla->cuentas[i].saldo -= retirada;
                tabla->cuentas[i].num_transacciones++;
                buffer_push(tabla->cuentas[i]);
                printf("Retirada exitosa. Nuevo saldo: %.2f\n", tabla->cuentas[i].saldo);
            } 
            else 
            {
                printf("Saldo insuficiente.\n");
                retirada = 0;
            }
            break;
        }
    }

    if (!encontrado) 
    {
        printf("Cuenta no encontrada.\n");
        retirada = 0;
    }

    sem_post(sem_cuentas);
    return retirada;
}

float RealizarTransferencia(int NumCuenta) 
{
    sem_wait(sem_cuentas);

    int cuentaDestino;
    float cantidad;
    printf("Ingrese número de cuenta destino: ");
    scanf("%d", &cuentaDestino);
    printf("Ingrese cantidad a transferir: ");
    scanf("%f", &cantidad);

    int encontradaOrigen = 0, encontradaDestino = 0;
    int posOrigen = -1, posDestino = -1;

    // Buscar ambas cuentas
    for (int i = 0; i < tabla->num_cuentas; i++) 
    {
        if (tabla->cuentas[i].numero_cuenta == NumCuenta) 
        {
            encontradaOrigen = 1;
            posOrigen = i;
        }
        if (tabla->cuentas[i].numero_cuenta == cuentaDestino) 
        {
            encontradaDestino = 1;
            posDestino = i;
        }
    }

    if (!encontradaOrigen || !encontradaDestino) 
    {
        printf("❌ Alguna de las cuentas no existe.\n");
        sem_post(sem_cuentas);
        return 0;
    }

    if (cantidad > config.limite_transferencia) 
    {
        printf("Transferencia excede el límite permitido. Operación cancelada.\n");
        char mensaje[256];
        snprintf(mensaje, sizeof(mensaje), "CUENTA:%d|OPERACION:Transferencia ANÓMALA|CANTIDAD:%.2f", NumCuenta, cantidad);
        write(write_fd, mensaje, strlen(mensaje));
        sem_post(sem_cuentas);
        return 0;
    }

    if (tabla->cuentas[posOrigen].saldo < cantidad) 
    {
        printf("❌ Saldo insuficiente para realizar la transferencia.\n");
        sem_post(sem_cuentas);
        return 0;
    }

    // Transferencia válida
    tabla->cuentas[posOrigen].saldo -= cantidad;
    tabla->cuentas[posOrigen].num_transacciones++;
    tabla->cuentas[posDestino].saldo += cantidad;
    tabla->cuentas[posDestino].num_transacciones++;

    buffer_push(tabla->cuentas[posOrigen]);
    buffer_push(tabla->cuentas[posDestino]);

    printf("\n✅ Transferencia realizada con éxito.\n");
    printf("➡️  %.2f€ enviados de cuenta %d a cuenta %d.\n", cantidad, NumCuenta, cuentaDestino);

    sem_post(sem_cuentas);
    return cantidad;
}

void *realizar_operacion(void *arg) {
    pthread_mutex_lock(&mutex_operaciones);

    int opcion = ((int*)arg)[0];
    int NumCuenta = ((int*)arg)[1];

    float cantidad;

    switch (opcion) {
        case 1:
            printf("Depósito seleccionado.\n");
            cantidad = RealizarDeposito(NumCuenta);
            if(cantidad > 0)
                RegistrarOperaciones(NumCuenta, cantidad, "Depósito");
            break;
        case 2:
            printf("Retirada seleccionada.\n");
            cantidad = RealizarRetirada(NumCuenta);
            if(cantidad > 0)
                RegistrarOperaciones(NumCuenta, -cantidad, "Retirada");
            break;
        case 3:
            printf("Transferencia seleccionada.\n");
            cantidad = RealizarTransferencia(NumCuenta);
            if(cantidad > 0)
                RegistrarOperaciones(NumCuenta, -cantidad, "Transferencia");
            break;
        case 4:
            printf("Consulta de saldo seleccionada.\n");    
            RealizarConsulta(NumCuenta);
            break;
        default:
            printf("Opción no válida.\n");
            break;
    }

    pthread_mutex_unlock(&mutex_operaciones);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <fd_tuberia_write> <shm_id>\n", argv[0]);
        exit(1);
    }

    write_fd = atoi(argv[1]);
    shm_id = atoi(argv[2]);

    tabla = (TablaCuentas *)shmat(shm_id, NULL, 0);
    if (tabla == (void *)-1) 
    {
        perror("Error al hacer shmat en usuario");
        exit(1);
    }

    int opcionmenu = 1;
    pthread_t hilo;
    int opcion = 0;
    int NumCuenta;

    // Accedemos al semaforo nombrado de banco.c
    sem_cuentas = sem_open("/cuentas_sem", 0);
    if (sem_cuentas == SEM_FAILED) 
    {
        perror("Error al abrir el semáforo.");
        exit(1);
    }

    pthread_mutex_init(&mutex_operaciones, NULL);
    config = leer_configuracion(CONFIG_PATH);

    printf("=========================================\n");
    printf("        🧾 Bienvenido al banco 🧾\n");
    printf("=========================================\n\n");

    printf("🔑 Introduce tu número de cuenta: ");
    scanf("%d", &NumCuenta);

    while (opcionmenu == 1) {
        printf("\n");
        printf("╔═════════════════════════════════════════╗\n");
        printf("║         🏦  BANCO TERMINAL INTERACTIVO   ║\n");
        printf("╠══════════════════════════════════════════╣\n");
        printf("║  1. 💰 Depósito                         ║\n");
        printf("║  2. 🏧 Retirada                         ║\n");
        printf("║  3. 🔄 Transferencia                    ║\n");
        printf("║  4. 📄 Consultar saldo                  ║\n");
        printf("║  5. 🚪 Salir                            ║\n");
        printf("╚══════════════════════════════════════════╝\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);

        if (opcion >= 1 && opcion <= 4) {
            pthread_create(&hilo, NULL, realizar_operacion, (void *)&(struct {int opcion; int cuenta;}){opcion, NumCuenta});
            pthread_join(hilo, NULL);
        } else if (opcion == 5) {
            printf("\n👋 ¡Gracias por usar el banco! ¡Hasta pronto!\n");
            opcionmenu = 0;
        } else {
            printf("❌ Opción inválida. Intente de nuevo.\n");
        }
    }

    sem_close(sem_cuentas);
    pthread_mutex_destroy(&mutex_operaciones);
    return 0;
}