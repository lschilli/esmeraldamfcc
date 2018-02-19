/**
* Datei:	dsp.h
* Autor:	Gernot A. Fink
* Datum:	10.4.1997
*
* Beschreibung:	Definitionen von Datenstrukturen und globalen
*		Variablen fuer digitale Signalverarbeitungsroutinen
**/

#ifndef __DSP_H_INCLUDED__
#define __DSP_H_INCLUDED__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <mx/real.h>
#include <mx/complex.h>

#define DSP_MK_VERSION(major, minor)	((major) << 8 | (minor))
#define DSP_VERSION_MAJOR(version)	((version) >> 8)
#define DSP_VERSION_MINOR(version)	((version) & 0xff)

#define	DSP_EPS		(1e-10)
#define	DSP_LOGEPS	(-23)
#define	DSP_LOG10EPS	(-10)

#define	dsp_log(x)	(((x) > DSP_EPS) ? log(x) : DSP_LOGEPS)
#define	dsp_log10(x)	(((x) > DSP_EPS) ? log10(x) : DSP_LOG10EPS)

#define dsp_sqr(x)	((x) * (x))

#define dsp_min(a, b)	(((a) < (b)) ? (a) : (b))
#define dsp_max(a, b)	(((a) > (b)) ? (a) : (b))

extern char *program;		/* Programmname o.Ae. fuer Fehlermeldungen */

/**
* Datentypen
**/

typedef short dsp_sample_t;	/* allgemeiner Abtastwert */

#ifdef DSP_KERNEL
#include "dct.h"
#include "delay.h"
#include "channel.h"
#include "fextract.h"
#include "dwt.h"
#include "lpc.h"
#include "filterbank.h"
#else
#include <dsp/dct.h>
#include <dsp/delay.h>
#include <dsp/channel.h>
#include <dsp/fextract.h>
#include <dsp/dwt.h>
#include <dsp/lpc.h>
#include <dsp/filterbank.h>
#endif

/**
* Funktionsprototypen
**/

/* Praeemphase */
dsp_sample_t dsp_preemph(dsp_sample_t *p, dsp_sample_t *s, size_t n,
		mx_real_t a, dsp_sample_t s_minus_1);

/* Fensterfunktionen */
void dsp_window_hamming(mx_real_t *w, dsp_sample_t *s, size_t n);
void dsp_window_hanning(mx_real_t *w, dsp_sample_t *s, size_t n);
mx_real_t* dsp_window_hanning_create(size_t n);

/* FFT */
void dsp_xfft(mx_complex_t *, size_t n, int sign);
void dsp_xdft(mx_complex_t *, size_t n, int sign);

/* gehoerrichtige Verzerrung der Frequenzachse */
mx_real_t dsp_f_width(mx_real_t f);
int dsp_mel_create(mx_real_t *mel_freq, mx_real_t *f_width,
		mx_real_t d_freq, mx_real_t f_min, mx_real_t f_max,
		mx_real_t scale, int max_fgroups);

/* diskrete zeitliche Glaettung */
int dsp_tirol(mx_real_t *tirol, dsp_delay_t *w);

/* diskrete zeitliche Ableitungen */
int dsp_deriv(mx_real_t *dx, dsp_delay_t *w, int r, mx_real_t gain);

#endif /* __DSP_H_INCLUDED__ */
