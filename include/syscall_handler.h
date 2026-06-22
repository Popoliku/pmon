#ifndef SYSCALL_HANDLER_H
#define SYSCALL_HANDLER_H

#include <sys/types.h>

/**
 * @brief 	   Lee un bloque de memoria del espacio de direcciones del proceso monitorizado.
 * @param pid      Identificador del proceso (PID) al que se le va a leer la memoria.
 * @param org      Puntero a la direccion de memoria de origen dentro del proceso hijo.
 * @param dst	   Puntero al buffer de destino en el proceso monitor donde se guardaran los datos.
 * @param n        Numero de bytes totales que se desean leer.
 */
void peek_proc_mem(pid_t pid, const void* org, void* dst, size_t n);

/**
 * @brief 	   Lee una cadena de caracteres (string) de la memoria del proceso monitorizado hasta encontrar el caracter nulo.
 * @param pid      Identificador del proceso (PID) del cual se va a extraer la cadena.
 * @param org      Puntero a la direccion de memoria dentro del proceso hijo donde esta la cadena.
 * @return         Un puntero a una cadena de caracteres, debe ser liberada con free() por el llamador. Retorna NULL en caso de error.
 */
char * peek_proc_string(pid_t pid, const void *org);

/**
 * @brief 	      Intercepta la syscall del proceso monitorizado y printea sus argumentos y el valor de retorno.
 * @param pid         Identificador del proceso (PID) que ha disparado la llamada al sistema.
 * @param status      Puntero al entero que contiene el estado de parada actual reportado por waitpid().
 * @param regs        Puntero a la estructura que contiene los registros de la CPU del proceso. 
 * @return 	      Un entero que representa si la syscall era una de interes o no.	   
 */
int handle_syscall(pid_t pid, int* status, struct user_regs_struct* regs);

#endif
