#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h> 
#include <pthread.h>
#include <signal.h>
#include "utils.h"
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_USUARIOS 5


#define ARCHIVO_CUENTAS "../data/cuentas.txt"
#define ARCHIVO_CONFIG "../data/config.txt"
#define ARCHIVO_TRANSACCIONES "../data/transacciones.log"


sem_t *semaforo = NULL;
int pipes_hijo_padre[MAX_USUARIOS][2];
int numCuenta;
struct mensaje {
    long tipo;
    char texto[256];
};

Config leer_configuracion(const char *ruta) 
{
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
    printf("Se ha cargado el archivo 'config.txt' correctamente.\n");
    return config;
}

sem_t *InicializarSemaforo() {
    semaforo = sem_open("/cuentas_sem", O_CREAT, 0644, 1);
    if(semaforo == SEM_FAILED) {
        perror("Error al crear el semáforo.");
        return SEM_FAILED;
    }
    printf("Se ha inicializado el semáforo correctamente.\n");
    return semaforo;
}

int InicializarColaMensajes() {
    key_t key = ftok("/tmp", 'A');
    if (key == -1) {
        perror("Error al generar la clave con ftok");
        exit(1);
    }
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1){
        perror("Error al crear o abrir la cola de mensajes");
        exit(1);
    }
    return msgid;
}

void *EscucharAlertasDesdeMonitor(void *arg) {
    const char *pipe_monitor_banco = "/tmp/monitor_a_banco";
    mkfifo(pipe_monitor_banco, 0666);

    int fd_monitor = open(pipe_monitor_banco, O_RDONLY);
    if (fd_monitor == -1) {
        perror("[Banco] Error al abrir tubería desde monitor");
        pthread_exit(NULL);
    }

    char alerta[256];
    while (1) {
        ssize_t bytes = read(fd_monitor, alerta, sizeof(alerta) - 1);
        if (bytes > 0) {
            alerta[bytes] = '\0';
            printf("[BANCO] 🔴 ALERTA RECIBIDA: %s\n", alerta);
            FILE *log = fopen("../data/alertas.log", "a");
            if (log) {
                time_t now = time(NULL);
                char *timestamp = ctime(&now);
                timestamp[strcspn(timestamp, "\n")] = 0;
                fprintf(log, "[%s] %s\n", timestamp, alerta);
                fclose(log);
            }
        }
    }

    close(fd_monitor);
    pthread_exit(NULL);
}

void EscucharTuberias(int pipes_hijo_padre[MAX_USUARIOS][2], int cola_mensajes) {
    fd_set read_fds;
    int max_fd = 0;
    char buffer[256];

    int usuarios_activos = MAX_USUARIOS;
    int cerrado[MAX_USUARIOS] = {0};

    printf("Escuchando mensajes de los hijos...\n");

    while (usuarios_activos > 0) {
        FD_ZERO(&read_fds);
        max_fd = 0;

        for (int i = 0; i < MAX_USUARIOS; i++) {
            if (!cerrado[i]) {
                FD_SET(pipes_hijo_padre[i][0], &read_fds);
                if (pipes_hijo_padre[i][0] > max_fd) {
                    max_fd = pipes_hijo_padre[i][0];
                }
            }
        }

        int actividad = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (actividad < 0) {
            perror("Error al usar la función 'select()'");
            break;
        }

        for (int i = 0; i < MAX_USUARIOS; i++) {
            if (!cerrado[i] && FD_ISSET(pipes_hijo_padre[i][0], &read_fds)) {
                ssize_t bytes_leidos = read(pipes_hijo_padre[i][0], buffer, sizeof(buffer) - 1);

                if (bytes_leidos > 0) {
                    buffer[bytes_leidos] = '\0';

                    // Extraer el número de cuenta
                    int cuenta_numero = -1;
                    char *inicio = strstr(buffer, "CUENTA:");
                    if (inicio != NULL) {
                        cuenta_numero = atoi(inicio + 7);
                    }

                    // Obtener timestamp
                    time_t now = time(NULL);
                    char *timestamp = ctime(&now);
                    timestamp[strcspn(timestamp, "\n")] = 0;

                    // Escribir en el log del usuario si se pudo extraer la cuenta
                    if (cuenta_numero > 0) {
                        char ruta_log_usuario[256];
                        snprintf(ruta_log_usuario, sizeof(ruta_log_usuario),
                                 "../data/transacciones/%d/transacciones.log", cuenta_numero);

                        FILE *log_usuario = fopen(ruta_log_usuario, "a");
                        if (log_usuario) {
                            fprintf(log_usuario, "[%s] %s\n", timestamp, buffer);
                            fclose(log_usuario);
                        } else {
                            perror("Error al abrir log individual de usuario");
                        }
                    } else {
                        // Si no hay cuenta en el mensaje, lo escribimos en el log general
                        FILE *log_file = fopen("../data/transacciones.log", "a");
                        if (log_file) {
                            fprintf(log_file, "[%s] %s\n", timestamp, buffer);
                            fclose(log_file);
                        }
                    }

                    // Enviar a monitor
                    struct mensaje msg;
                    msg.tipo = 1;
                    strncpy(msg.texto, buffer, sizeof(msg.texto) - 1);
                    msg.texto[sizeof(msg.texto) - 1] = '\0';

                    if (msgsnd(cola_mensajes, &msg, sizeof(msg.texto), 0) == -1) {
                        perror("Error al enviar mensaje al monitor");
                    }

                } else if (bytes_leidos == 0) {
                    time_t now = time(NULL);
                    char *timestamp = ctime(&now);
                    timestamp[strcspn(timestamp, "\n")] = 0;

                    FILE *log_file = fopen("../data/transacciones.log", "a");
                    if (log_file) {
                        fprintf(log_file, "[%s] Usuario %d ha cerrado sesión.\n", timestamp, i + 1);
                        fclose(log_file);
                    }

                    printf("El usuario %d ha cerrado sesión.\n", i + 1);
                    cerrado[i] = 1;
                    close(pipes_hijo_padre[i][0]);
                    usuarios_activos--;
                }
            }
        }
    }
    printf("Todos los usuarios han cerrado sesión. Finalizando escucha de tuberías...\n");
}


void actualizar_archivo_cuentas(const char *archivo_cuentas, TablaCuentas *tabla)
{
    FILE *archivo = fopen(archivo_cuentas, "w");

    if (archivo == NULL) 
    {
        perror("Error al abrir cuentas.txt para volcado");
        return;
    }

    for (int i = 0; i < tabla->num_cuentas; i++) 
    {
        Cuenta *cuenta = &tabla->cuentas[i];
        fprintf(archivo, "%d,%s,%.2f,%d\n",
                cuenta->numero_cuenta,
                cuenta->titular,
                cuenta->saldo,
                cuenta->num_transacciones);
    }

    fclose(archivo);
    printf("Se ha realizado el volcado de la memoria compartida a cuentas.txt correctamente.\n");
}

// Función para gestionar finalización forzada del sistema
void manejar_salida_forzada(int sig) {
    printf("\nSeñal de salida recibida (SIGINT). Guardando datos...\n");
    actualizar_archivo_cuentas(ARCHIVO_CUENTAS, tabla);
    liberar_memoria_compartida();
    sem_unlink("/cuentas_sem");
    printf("Datos guardados. Finalizando el sistema...\n");
    exit(0);
}

void cargar_tabla_cuentas(const char *archivo_cuentas, TablaCuentas *tabla)
{
    FILE *archivo = fopen(archivo_cuentas, "r");
    if (archivo == NULL) {
        perror("Error al abrir cuentas.txt");
        exit(1);
    }

    char linea[256];
    int i = 0;

    while (fgets(linea, sizeof(linea), archivo) && i < 100) {
        int numero;
        char titular[50];
        float saldo;
        int transacciones;

        // Separar la línea por comas
        char *token = strtok(linea, ",");
        if (token != NULL) numero = atoi(token);

        token = strtok(NULL, ",");
        if (token != NULL) strncpy(titular, token, sizeof(titular));
        titular[sizeof(titular) - 1] = '\0';  // aseguramos terminación nula

        token = strtok(NULL, ",");
        if (token != NULL) saldo = atof(token);

        token = strtok(NULL, ",");
        if (token != NULL) transacciones = atoi(token);

        // Guardar en la tabla de memoria compartida
        tabla->cuentas[i].numero_cuenta = numero;
        strncpy(tabla->cuentas[i].titular, titular, sizeof(tabla->cuentas[i].titular));
        tabla->cuentas[i].titular[sizeof(tabla->cuentas[i].titular) - 1] = '\0'; // Seguridad extra
        tabla->cuentas[i].saldo = saldo;
        tabla->cuentas[i].num_transacciones = transacciones;

        i++;
    }

    tabla->num_cuentas = i;
    fclose(archivo);
}


int main() {
    Config config;
    int cola_mensajes;
    int i;
    pid_t pid, monitor;
    pthread_t hilo_alertas;
    pthread_t hilo_entrada_salida;
    // Inicizalizamos
    crear_carpetas_transacciones();
    tabla = crear_memoria_compartida();
    signal(SIGINT, manejar_salida_forzada);
    cargar_tabla_cuentas(ARCHIVO_CUENTAS, tabla);
    int shm_id = obtener_shm_id();
    semaforo = InicializarSemaforo();
    cola_mensajes = InicializarColaMensajes();
    config = leer_configuracion(ARCHIVO_CONFIG);

    monitor = fork();
    if (monitor < 0) {
        perror("Error al crear el proceso monitor");
        exit(1);
    }
    if (monitor == 0) {
        execlp("./monitor", "monitor", NULL);
        perror("Error al ejecutar monitor");
        exit(1);
    }

    pthread_create(&hilo_alertas, NULL, EscucharAlertasDesdeMonitor, NULL);
    pthread_create(&hilo_entrada_salida, NULL, gestionar_entrada_salida, NULL);

    for (i = 0; i < MAX_USUARIOS; i++) {
        if (pipe(pipes_hijo_padre[i]) == -1) {
            perror("Error al crear alguna tubería entre proceso padre y procesos hijos");
            exit(1);
        }

        pid = fork();
        if (pid < 0) {
            perror("Error al crear proceso...");
            exit(1);
        }
        if (pid == 0) // Proceso hijo
        {
            // Cerramos extremo de lectura 
            close(pipes_hijo_padre[i][0]);
            char write_pipe_str[10];
            snprintf(write_pipe_str, sizeof(write_pipe_str), "%d", pipes_hijo_padre[i][1]);

            char shm_id_str[10];
            snprintf(shm_id_str, sizeof(shm_id_str), "%d", shm_id);

            // Pasamos como argumentos las tuberías entre banco y usuarios y el identificador de memoria compartida
            execlp("xterm", "xterm", "-e", "./usuario", write_pipe_str, shm_id_str, NULL);
            perror("Error al ejecutar usuario");
            exit(1);
        }
        close(pipes_hijo_padre[i][1]);
        
    }

    EscucharTuberias(pipes_hijo_padre, cola_mensajes);

    pthread_join(hilo_alertas, NULL);
    pthread_join(hilo_entrada_salida, NULL);

    for (i = 0; i < MAX_USUARIOS; i++) 
    {
        wait(NULL);
    }

    // Al finalizar el sistema, sincronizamos los datos de la memoria compartida con el fichero de cuentas y liberamos la memoria
    actualizar_archivo_cuentas(ARCHIVO_CUENTAS, tabla);
    liberar_memoria_compartida();
    sem_unlink("/cuentas_sem");
    sem_close(semaforo);
    return 0;
}