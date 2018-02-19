/**
* File:		filterbank.h
* Author:	Gernot A. Fink
* Date:		28.2.2008
*
* Description:	Definitions for filterbanks
*
**/

#ifndef __DSP_FILTERBANK_H_INCLUDED__
#define __DSP_FILTERBANK_H_INCLUDED__

#include <mx/real.h>

/*
 * Daten Types
 */
typedef struct {		/* Filterbank mit ... */
	int n_channels;		/* ... # der Filterkomponenten */
	int *left_ind;		/* ... 1. Index mit Filtergewicht != 0 ... */
	int *n_inds;		/* ... gefolgt von 'n_inds - 1' weiteren ... */
	mx_real_t **weight;	/* ... und Gewichte != 0 je Filterkomponente */
	} dsp_filterbank_t;

/*
 * Function Prototypes
 */
dsp_filterbank_t *dsp_filterbank_create(int n_channels,
                mx_real_t *mid_freq, mx_real_t *plateau,
		mx_real_t d_freq, mx_real_t f_min, mx_real_t f_max);
int dsp_filterbank_apply(mx_real_t *e, mx_real_t *f, dsp_filterbank_t *fb);
int dsp_filterbank_synth(mx_real_t *f, mx_real_t *e, dsp_filterbank_t *fb);

#endif /* __DSP_FILTERBANK_H_INCLUDED__ */
