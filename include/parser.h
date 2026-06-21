#ifndef PARSER_H
#define PARSER_H

#include <stdio.h> // Necesario porque usas FILE* en la firma de la función

/**
 * @brief Analiza una línea de entrada y la divide en un arreglo de tokens.
 * * @param stream      El flujo de entrada (ej. stdin).
 * @param comando     Puntero al buffer donde se guardará la línea leída (será modificado).
 * @param pmon_argc   Puntero a un entero donde se almacenará el número de argumentos.
 * @return char** Un arreglo de punteros a cadenas (argv). 
 * Debe ser liberado con free() por el llamador.
 */
char** get_input(FILE* stream, char** comando, int* pmon_argc);

#endif // PARSER_H
