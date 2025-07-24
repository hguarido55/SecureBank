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
│   ├── alertas.log
│   ├── config.txt
│   ├── cuentas.txt
│   ├── transacciones.log
│   └── transacciones/
│       ├── usuario1/
│       │   └── log.txt
│       ├── usuario2/
│       │   └── log.txt
│       └── ...
├── src/
│   ├── banco.c
│   ├── datos.c
│   ├── entrada_salida.c
│   ├── ficheros.c
│   ├── init_cuentas.c
│   ├── Launch.sh
│   ├── memoria.c
│   ├── monitor.c
│   ├── usuario.c
│   └── utils.h

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