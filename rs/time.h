/**
* Datei:	time.h
* Autor:	Thomas Ploetz, Mon Jun 28 10:45:54 2004
* Time-stamp:	<04/06/28 10:51:45 tploetz>
*
* Beschreibung:	Prototypendefinition für Zeitmessungen
**/

#ifndef __RS_TIME_H_INCLUDED__
#define __RS_TIME_H_INCLUDED__

#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>

/******************************** start time.h ***************************/

clock_t rs_timeval_diff(struct timeval *start, struct timeval *stop);
clock_t rs_tms_diff(struct tms *start, struct tms *stop);

/********************************** EOF time.h ***************************/
#endif /* __RS_TIME_H_INCLUDED__ */
