/**
* Datei:	eigen.h
* Autor:	Thomas Ploetz, Tue Aug  7 17:23:15 2001
* Time-stamp:	<01/08/08 13:45:36 tploetz>
*
* Beschreibung:	Definitionen von Datenstrukturen zur Eigenwertberechnung
**/

#ifndef _MX_EIGEN_H_INCLUDED_
#define _MX_EIGEN_H_INCLUDED_

#ifdef MX_KERNEL
#include "real.h"
#include "vector.h"
#else
#include <mx/real.h>
#include <mx/vector.h>
#endif

/* Kompatibilitaetsdefine */
#define mx_matrix_eigenw(ew,ev,a,dim) mx_matrix_eigenv(ew,ev,a,dim)

int mx_matrix_eigenv(mx_real_t **_ew, 
		     mx_real_t ***_ev,
		     mx_real_t  **a, 
		     int dim);

int mx_matrix_eigenv_n(mx_real_t **_ew, 
		       mx_real_t ***_ev,
		       mx_real_t **a, 
		       int dim,
		       int n);

#endif /* _MX_EIGEN_H_INCLUDED_ */
