/**
* Datei:	__program.c
* Autor:	Gernot A. Fink
* Datum:	10.4.1997
*
* Beschreibung:	Fall-Back-Loesung fuer die Definition der globalen
*		Variable 'program', wenn diese NICHT durch das
*		betreffende Hauptprogramm definiert wird.
**/

#define MX_KERNEL
#include "basics.h"

char *program = "[math extension library " MX_VERSION "]";

char *__program_get(void)
	{
	return(program);
	}
