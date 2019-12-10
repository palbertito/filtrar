/*
 * filtrar: filtra por contenido los archivos del directorio indicado.
 *
 * Copyright (c) 2013,2017 Francisco Rosales <frosal@fi.upm.es>
 * Todos los derechos reservados.
 *
 * Publicado bajo Licencia de Proyecto Educativo Práctico
 * <http://laurel.datsi.fi.upm.es/~ssoo/LICENCIA/LPEP>
 *
 * Queda prohibida la difusión total o parcial por cualquier
 * medio del material entregado al alumno para la realización 
 * de este proyecto o de cualquier material derivado de este, 
 * incluyendo la solución particular que desarrolle el alumno.
 */

/* 
   Nombre:				Alberto
   Apellidos:			Doncel Aparicio
   Numero de matrícula: y160364
   DNI: 				53715278C
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "filtrar.h"

/* Este filtro deja pasar los caracteres NO alfabeticos. */
/* Devuelve el numero de caracteres que han pasado el filtro. */
int tratar(char* buff_in, char* buff_out, int tam)
{
	int res = 0; 
	int j;
	for(j=0;j<tam;j++){
	if(!(isalpha(buff_in[j]))&&(buff_in[j]!='\0')){		/* dejamos pasar unicamente valores que no sean ni alfanumericos ni NULL */
		buff_out[res]=buff_in[j];						/* los almacenamos en buff_out */
		res++;											/* contamos el numero de valores leidos */
		}
	}
	return res;											/* devolvemos el numero de valores leidos */
}
