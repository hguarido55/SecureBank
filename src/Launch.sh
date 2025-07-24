gcc banco.c memoria.c ficheros.c entrada_salida.c datos.c -o banco -pthread -lrt
gcc usuario.c memoria.c ficheros.c entrada_salida.c datos.c -o usuario -pthread -lrt
gcc monitor.c memoria.c ficheros.c datos.c -o monitor -pthread -lrt
chmod +x usuario
./banco