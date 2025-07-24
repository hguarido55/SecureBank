# 🏦 SecureBank — Sistema Bancario Concurrente en C

Simulador avanzado de un sistema bancario concurrente diseñado para ejecutarse en un entorno Linux con fines académicos.

---

## ⚙️ Tecnologías y conceptos clave

- 🧠 **Lenguaje C** — Control de bajo nivel y manipulación eficiente del sistema  
- 🐧 **Linux** — Uso de multitarea, señales, tuberías y colas de mensajes  
- 🧵 **Hilos POSIX** — Para permitir múltiples operaciones simultáneas por usuario  
- 👥 **Procesos Hijos** — Cada usuario funciona como un proceso independiente  
- 🔐 **Sincronización** — Semáforos, mutexes y exclusión mutua para consistencia de datos  
- 🗄️ **Memoria Compartida** — Para compartir datos entre procesos de forma eficiente  
- 🔄 **Gestión de Entrada/Salida** — Manejo optimizado de operaciones de E/S  
- 📂 **Gestión de Sistema de Ficheros** — Control de archivos y logs con acceso sincronizado  
- 🚨 **Monitorización** — Análisis en tiempo real de transacciones sospechosas  
- 📋 **Logs de Auditoría** — Registro estructurado de todas las operaciones del sistema  

---

## 🏗️ Estructura del sistema

SecureBank/
├── data/
│ ├── alertas.log # Registro de operaciones sospechosas
│ ├── config.txt # Configuraciones iniciales del sistema
│ ├── cuentas.txt # Datos de cuentas bancarias iniciales
│ ├── transacciones.log # Log general de transacciones
│ └── transacciones/ # Logs individuales por usuario
│   ├── 1/
│   │ └── log_usuario1.txt
│   ├── 2/
│   │ └── log_usuario2.txt
│   ├── 3/
│   │ └── log_usuario3.txt
│   ├── 4/
│   │ └── log_usuario4.txt
│   └── 5/
│   └── log_usuario5.txt
│
├── src/
│ ├── banco.c # Proceso principal: coordina usuarios y monitor
│ ├── datos.c # Lectura de configuración y datos iniciales
│ ├── entrada_salida.c # Manejo de entrada y salida
│ ├── ficheros.c # Operaciones sobre archivos del sistema
│ ├── init_cuentas.c # Inicialización de las cuentas
│ ├── memoria.c # Gestión de memoria compartida
│ ├── monitor.c # Análisis y detección de transacciones anómalas
│ ├── usuario.c # Interfaz de usuario y gestión de hilos
│ ├── utils.h # Cabecera con estructuras comunes
│ └── Launch.sh # Script bash para lanzar el sistema

---

## 👨‍💻 Autores

- Antonio Gallego Ortiz
- Adrián Zamorano Valero
- Hugo Guarido Domínguez
- Manuel Luna Jiménez
- José Polanco Andrés

---

## 🚀 Cómo ejecutar la práctica

Para ejecutar el sistema correctamente, abre una **terminal Bash en Linux** y sigue estos pasos desde la raíz del proyecto `SecureBank/`:

**bash**
cd src
chmod 777 Launch.sh
sudo ./Launch.sh