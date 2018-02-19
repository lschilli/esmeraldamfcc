/**
* Datei:	histogram.h
* Autor:	Gernot A. Fink
* Datum:	9.8.2000
*
* Beschreibung:	Definitionen fuer eindimensionale Histogramme
**/

#ifndef __MX_HISTOGRAM_H_INCLUDED__
#define __MX_HISTOGRAM_H_INCLUDED__

#include <stdio.h>
#include <limits.h>

#ifdef MX_KERNEL
#include "real.h"
#else
#include <mx/real.h>
#endif

	/* maximal zulaessige Anzahl von Histogrammeintraegen */
#define MX_HISTOGRAM_SIZE_MAX	SHRT_MAX

#define MX_HISTOGRAM_IDX_UNDEF	(-2)

/* Typ eines Histogramms ... */
typedef struct {
	mx_real_t min;
	mx_real_t max;
	mx_real_t resolution;
	int n_buckets;
	mx_real_t *bucket;
	mx_real_t n_samples;	/* # Samples OHNE Ueber-/Unterlaufwerte */
	mx_real_t tot_samples;	/* # aller Samples auch bei Limitierung */
	int sample_limit;
	short *idx_history;
	int idx_history_top;
	} mx_histogram_t;

/*
 * Funktionsprototypen 
 */
mx_histogram_t *mx_histogram_create(mx_real_t min, mx_real_t max,
				mx_real_t resolution);
void mx_histogram_destroy(mx_histogram_t *hg);
void mx_histogram_reset(mx_histogram_t *hg);
void mx_histogram_reset_bin(mx_histogram_t *hg, mx_real_t val);

int mx_histogram_limit_set(mx_histogram_t *hg, int sample_limit);

/*** mx_histogram_t *mx_histogram_fscan(FILE *fp); ***/
int mx_histogram_fprint(FILE *fp, mx_histogram_t *hg);

int mx_histogram_size(mx_real_t min, mx_real_t max, mx_real_t resolution);
mx_real_t mx_histogram_resolution(mx_real_t min, mx_real_t max, int size);

int mx_histogram_val2idx(mx_histogram_t *hg, mx_real_t val);
mx_real_t mx_histogram_idx2val(mx_histogram_t *hg, int idx);
int mx_histogram_update(mx_histogram_t *hg, mx_real_t val);
int mx_histogram_update_urange(mx_histogram_t *hg,
		mx_real_t min, mx_real_t max, mx_real_t weight);

int mx_histogram_count(mx_histogram_t *hg,
                mx_real_t val);

mx_real_t mx_histogram_prob(mx_histogram_t *hg, mx_real_t val);
mx_real_t mx_histogram_prob4idx(mx_histogram_t *hg, int idx);
mx_real_t mx_histogram_prob_le(mx_histogram_t *hg, mx_real_t val);
mx_real_t mx_histogram_prob_le4idx(mx_histogram_t *hg, int idx);
mx_real_t mx_histogram_prob_ge(mx_histogram_t *hg, mx_real_t val);
mx_real_t mx_histogram_invprob_le(mx_histogram_t *hg, mx_real_t prob);
mx_real_t mx_histogram_invprob_ge(mx_histogram_t *hg, mx_real_t prob);

mx_real_t mx_histogram_mean(mx_histogram_t *hg);
mx_real_t mx_histogram_variance(mx_histogram_t *hg);
mx_real_t mx_histogram_standard_deviation(mx_histogram_t *hg);
mx_real_t mx_histogram_mode(mx_histogram_t *hg);

mx_real_t mx_histogram_otsu_thresh(mx_real_t *m1p, mx_real_t *m2p,
		mx_histogram_t *hg);
#endif /* __MX_HISTOGRAM_H_INCLUDED__ */
