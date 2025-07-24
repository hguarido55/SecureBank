#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "utils.h"

#define MAX_USUARIOS 5

// Función para inicializar el archivo binario con cuentas

int main()
{
    FILE *ArchivoCuentas = fopen("../data/cuentas.txt","w");
    int i;

    if(ArchivoCuentas == NULL)
    {
        perror("Error al abrir el archivo");
        return 1;
    }

    Cuenta CuentasIniciales[MAX_USUARIOS] = {
        {1,"Jude Bellingham",2500.00,0},
        {2,"Zinedine Zidane",3500.00,0},
        {3,"Vinicius Jr",6500.00,0},
        {4,"Cristiano Ronaldo",1500.00,0},
        {5,"Sergio Ramos",1000.00,0}
    };

    // Escribir las cuentas iniciales en el archivo de cuentas
    for (int i = 0; i < MAX_USUARIOS; i++) 
    {
        fprintf(ArchivoCuentas, "%d,%s,%.2f,%d\n",
                CuentasIniciales[i].numero_cuenta,
                CuentasIniciales[i].titular,
                CuentasIniciales[i].saldo,
                CuentasIniciales[i].num_transacciones);
    }

    
    fclose(ArchivoCuentas);
    return 0;
}
