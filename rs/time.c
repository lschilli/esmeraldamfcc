/**
* Datei:	time.c
* Autor:	Thomas Ploetz, Mon Jun 28 10:43:49 2004
* Time-stamp:	<04/06/28 10:48:53 tploetz>
*
* Beschreibung:	Funktionen für Zeitmessungen
**/

#define RS_KERNEL
#include "time.h"

/************************************ Start ***********************************/

clock_t rs_timeval_diff(struct timeval *start, struct timeval *stop)
	{
	clock_t msecs;

	msecs = (stop->tv_sec - start->tv_sec) * 1000;
	msecs += (stop->tv_usec - start->tv_usec) / 1000;

	return(msecs);
	}

clock_t rs_tms_diff(struct tms *start, struct tms *stop)
	{
	clock_t msecs;

	msecs = (1000 * (stop->tms_utime - start->tms_utime)) /
			sysconf(_SC_CLK_TCK);

	return(msecs);
	}

/********************************* EOF time.c *********************************/
