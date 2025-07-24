#ifndef UTILS_H
#define UTILS_H

#define MAX_USUARIOS 5
#define BUFFER_MAX 10
#define NUM_CUENTAS 5

#include <pthread.h>

// ----- Estructuras -----

typedef struct Operacion {
    int numeroCuenta;
    float cantidad;
    char operacion[128];
} Operacion;

typedef struct Config {
    int limite_retiro;
    int limite_transferencia;
    int umbral_retiros;
    int umbral_transferencias;
    int num_hilos;
    char archivo_cuentas[50];
    char archivo_log[50];
} Config;

typedef struct Cuenta {
    int numero_cuenta;
    char titular[50];
    float saldo;
    int num_transacciones;
} Cuenta;

typedef struct {
    Cuenta cuentas[100];
    int num_cuentas;
} TablaCuentas;

typedef struct {
    Cuenta operaciones[BUFFER_MAX];
    int inicio;
    int fin;
} BufferEstructurado;

struct msgbuf {
    long mtype;
    char texto[100];
};

// ----- Declaraciones externas (después de definir structs) -----

extern TablaCuentas *tabla;
extern int shm_id;
extern Cuenta CuentasIniciales[NUM_CUENTAS];

// ----- Funciones -----

// Memoria compartida
TablaCuentas *crear_memoria_compartida();
void liberar_memoria_compartida();
int obtener_shm_id();
TablaCuentas *obtener_tabla();

// Sistema de ficheros
void crear_carpetas_transacciones();
void RegistrarOperaciones(int numeroCuenta, float cantidad, const char *operacion);

// Entrada/Salida
void buffer_push(Cuenta cuenta);
void *gestionar_entrada_salida(void *arg);
void sincronizar_cuenta_desde_disco(int numeroCuenta, TablaCuentas *tabla);

#endif