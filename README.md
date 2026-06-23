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
el flujo original del binario. Además, permite imprimir
la jerarquía de procesos así como los estados de los
descriptores de fichero de cada proceso

---

## 🛠️ Requisitos previos

El monitor requiere un entorno **Linux nativo de 64 bits** (x86_64).
Instala los siguientes paquetes esenciales:

```bash
sudo apt update
sudo apt install cmake build-essential
