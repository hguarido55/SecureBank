#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include "utils.h"

#define MAX_TEXT 256

struct mensaje {
    long tipo;
    char texto[MAX_TEXT];
};

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

void obtener_timestamp(char* buffer, size_t size) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buffer, size, "[%Y-%m-%d %H:%M:%S]", &tm);
}

void guardar_log(const char* texto) {
    FILE *f = fopen("transacciones.log", "a");
    if (f) {
        char timestamp[64];
        obtener_timestamp(timestamp, sizeof(timestamp));
        fprintf(f, "%s %s\n", timestamp, texto);
        fclose(f);
    } else {
        perror("No se pudo abrir transacciones.log");
    }
}

int detectar_anomalia(const char *mensaje, Config config) {
    if (strstr(mensaje, "Retirada")) {
        const char *cantidad_str = strrchr(mensaje, ':');
        if (cantidad_str != NULL) {
            float cantidad = atof(cantidad_str + 1);
            if (cantidad > config.limite_retiro) return 1;
        }
    }
    if (strstr(mensaje, "Transferencia")) {
        const char *cantidad_str = strrchr(mensaje, ':');
        if (cantidad_str != NULL) {
            float cantidad = atof(cantidad_str + 1);
            if (cantidad > config.limite_transferencia) return 1;
        }
    }
    if (strstr(mensaje, "Deposito")) {
        const char *cantidad_str = strrchr(mensaje, ':');
        if (cantidad_str != NULL) {
            float cantidad = atof(cantidad_str + 1);
            if (cantidad > 3 * config.limite_transferencia) return 1;
        }
    }
    return 0;
}

int main() {
    key_t key;
    int msgid;
    struct mensaje msg;
    Config config = leer_configuracion("../data/config.txt");

    key = ftok("/tmp", 'A');
    if (key == -1) {
        perror("Error al generar clave con ftok");
        exit(1);
    }

    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("Error al acceder a la cola de mensajes");
        exit(1);
    }

    const char *pipe_monitor_banco = "/tmp/monitor_a_banco";
    mkfifo(pipe_monitor_banco, 0666);

    printf("[Monitor] Esperando mensajes del banco...\n");

    while (1) {
        if (msgrcv(msgid, &msg, sizeof(msg.texto), 0, 0) == -1) {
            perror("Error al recibir mensaje");
            continue;
        }

        printf("[Monitor] Mensaje recibido: %s\n", msg.texto);
        guardar_log(msg.texto);

        if (detectar_anomalia(msg.texto, config)) {
            int fd_alerta = open(pipe_monitor_banco, O_WRONLY);
            if (fd_alerta != -1) {
                char alerta[512];  // ✅ Tamaño aumentado
                snprintf(alerta, sizeof(alerta), "ALERTA: Operación sospechosa detectada -> %s", msg.texto);
                write(fd_alerta, alerta, strlen(alerta));
                close(fd_alerta);
                printf("\033[1;31m[MONITOR] 🔴 ALERTA DETECTADA: %s\033[0m\n", alerta);  // Rojo fuerte
                printf("[Monitor] Alerta enviada al banco.\n");
                guardar_log(alerta);                
            } else {
                perror("[Monitor] Error al abrir la tubería para enviar alerta al banco");
            }
        }
    }

    return 0;
}
