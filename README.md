# pmon (Process Monitor)

Monitor interactivo multiproceso basado en `ptrace` 
para sistemas Linux (x86_64).

---

## 📋 ¿De qué va el proyecto?

**pmon** es una herramienta interactiva para monitorizar
y trazar llamadas al sistema (syscalls) en Linux. 

Permite ejecutar un programa objetivo y rastrear de forma 
automática toda su descendencia (procesos hijos generados 
mediante `fork`). Intercepta de forma segura 
las fases de entrada y salida de cada syscall sin alterar 
el flujo original del binario. 

---

## 🛠️ Requisitos previos

El monitor requiere un entorno **Linux nativo de 64 bits** (x86_64).
Instala los siguientes paquetes esenciales:

```bash
sudo apt update
sudo apt install cmake build-essential

---

## 📥 Descarga

Clona el repositorio:

git clone https://github.com/Popoliku/pmon
cd pmon

---

## 🔨 Compilación

La forma recomendada de compilar el proyecto es mediante el Makefile:

make

Esto configurará automáticamente el directorio de build y compilará pmon junto con los tests.

Alternativamente, el proyecto también puede compilarse manualmente con CMake:

cmake -S . -B build
cmake --build build

Esto generará:

build/pmon → ejecutable principal
binarios de tests
programas auxiliares de testing

---

## ▶️ Ejecución

Ejecutar pmon:

make run

O directamente:

./build/pmon <programa> [argumentos...]

Ejemplo:

./build/pmon /bin/ls -l

Durante la ejecución, PMON permite interactuar mediante comandos desde la terminal:

cont <pid> → reanuda el proceso hasta la siguiente syscall e imprime por pantalla los argumentos y el valor de retorno
ps         → imprime la jerarquía de procesos así como el estado de los descriptores de ficheros de cada proceso
quit       → sale de pmon

---

## 🧪 Tests

Ejecutar todos los tests:

make test

También pueden ejecutarse manualmente con CTest:

ctest --test-dir build --output-on-failure

Ejecutar un test específico:

./build/test_input
./build/test_syscall_handler
./build/test_pr_tree
./build/test_fd_table

---

## 📁 Estructura del proyecto
.
├── include/        # Headers públicos
├── src/            # Implementación principal
├── test/           # Unit tests y programas auxiliares
├── build/          # Archivos generados por CMake
├── CMakeLists.txt
├── Makefile
└── README.md

---

## 🧹 Limpieza

Eliminar archivos generados:

make clean

---

## 📝 Notas

pmon está desarrollado y probado en Linux.
El monitor utiliza ptrace, por lo que requiere permisos adecuados para trazar procesos.
Algunos tests dependen de comportamiento específico del kernel Linux.
