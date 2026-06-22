#ifndef PARSER_H
#define PARSER_H

#include <stdio.h> 

/**
 * @brief Obtiene el input del usuario y lo guarda en un array de strings (para separar los argumentos) 
 * @param stream      Desde donde va a leer
 * @param comando     Puntero al buffer donde se guardara la linea leida (input del usuario).
 * @param pmon_argc   Puntero a un entero donde se almacenara el numero de argumentos.
 * @return 	      Un array de strings (argv). 
 * Debe ser liberado con free() por el llamador.
 */
char** get_input(FILE* stream, char** comando, int* pmon_argc);

#endif 
