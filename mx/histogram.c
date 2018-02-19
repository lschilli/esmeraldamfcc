/**
* Datei:	histogram.c
* Autor:	Gernot A. Fink
* Datum:	9.8.2000
*
* Beschreibung:	Algorithmen fuer eindimensionale Histogramme
**/

#include <string.h>

#include <rs/memory.h>
#include <rs/messages.h>

#define MX_KERNEL
#include "histogram.h"

mx_histogram_t *mx_histogram_create(mx_real_t min, mx_real_t max,
		mx_real_t resolution)
	{
	mx_histogram_t *hg;

	/* Parameter pruefen ... */
	if (min >= max)
		rs_error("invalid histogram range [%g..%g]!", min, max);
	if (resolution <= 0)
		rs_error("invalid histogram resolution %g!", resolution);
	if ((max - min) / resolution > MX_HISTOGRAM_SIZE_MAX)
		rs_error("can't create excess %d bucket histogram!",
			(int)((max - min) / resolution));

	/* ... Speicher bereistellen und initialisieren ... */
	hg = rs_malloc(sizeof(mx_histogram_t), "histogram");

	hg->min =		min;
	hg->max =		max;
	hg->resolution =	resolution;

	hg->n_buckets = (max - min) / resolution + 1;
		/*
		 * Listenlaenge wird so alloziert, damit sowohl
		 * bucket[-1] und bucket[n_buckets] existieren, die zur
		 * Zaehlung von "Ausreissern" jenseits min und max dienen
		 */
	hg->bucket = rs_calloc(hg->n_buckets + 2, sizeof(mx_real_t),
				"histogram buckets");
	hg->bucket++;
	hg->n_samples = 0;
	hg->tot_samples = 0;

	/* ... sowie per default KEINE Begrenzung der "Geschichte" */
	hg->sample_limit = 0;
	hg->idx_history = NULL;
	hg->idx_history_top = 0;

	return(hg);
	}

void mx_histogram_destroy(mx_histogram_t *hg)
	{
	if (!hg)
		return;

	if (hg->bucket)
		rs_free(hg->bucket - 1);

	memset(hg, -1, sizeof(mx_histogram_t));
	rs_free(hg);
	}

void mx_histogram_reset(mx_histogram_t *hg)
	{
	int i;

	if (!hg || !hg->bucket)
		return;

	for (i = -1; i <= hg->n_buckets; i++)
		hg->bucket[i] = 0;
	hg->n_samples = 0;
	}

void mx_histogram_reset_bin(mx_histogram_t *hg, mx_real_t val)
	{
	int idx;

	if (!hg || !hg->bucket)
		return;

	idx = mx_histogram_val2idx(hg, val);

	hg->n_samples -= hg->bucket[idx];
	hg->bucket[idx] = 0;
	}

int mx_histogram_limit_set(mx_histogram_t *hg, int sample_limit)
	{
	int i;

	if (!hg || sample_limit <= 0)
		return(-1);

	if (hg->n_samples > sample_limit)
		return(-1);

	if (hg->idx_history)
		rs_error("can't re-configure histogram index history!");
	else	{
		hg->sample_limit = sample_limit;
		hg->idx_history = rs_malloc(sizeof(short) * sample_limit,
					"histogram index history");
		for (i = 0; i < hg->sample_limit; i++)
			hg->idx_history[i] = MX_HISTOGRAM_IDX_UNDEF;

		hg->idx_history_top = 0;
		}

	return(hg->sample_limit);
	}

/*** mx_histogram_t *mx_histogram_fscan(FILE *fp); ***/

int mx_histogram_fprint(FILE *fp, mx_histogram_t *hg)
	{
	int i;

	if (!hg)
		return(-1);

	fprintf(fp, "{l,%g..%g,h}/%g = [", hg->min, hg->max, hg->resolution);
	for (i = -1; i <= hg->n_buckets; i++)
		fprintf(fp, "%4.1f ",
			(hg->n_samples) > 0 ?
				100 * hg->bucket[i] / hg->n_samples : 0.0);
	fprintf(fp, "] (%g)\n", hg->n_samples);

	return(0);
	}

int mx_histogram_size(mx_real_t min, mx_real_t max,
		mx_real_t resolution)
	{
	int size;

	size = (max - min) / resolution;

	return size;
	}

mx_real_t mx_histogram_resolution(mx_real_t min,
		mx_real_t max, int size)
	{
	mx_real_t resolution;

	resolution = (max - min) / (float)size;

	return resolution;
	}

int mx_histogram_val2idx(mx_histogram_t *hg, mx_real_t val)
	{
	int idx;

	if (!hg)
		return(MX_HISTOGRAM_IDX_UNDEF);

	if (val > (hg->max + hg->resolution / 2))
		idx = hg->n_buckets;
	else if (val < (hg->min - hg->resolution / 2))
		idx = -1;
	else	idx = (int)(((val - hg->min) / hg->resolution) + 0.5);

	return(idx);
	}

mx_real_t mx_histogram_idx2val(mx_histogram_t *hg, int idx)
	{
	if (idx < -1)
		idx = -1;
	else if (idx > hg->n_buckets)
		idx = hg->n_buckets;

	return(hg->min + idx * hg->resolution);
	}

int mx_histogram_update(mx_histogram_t *hg, mx_real_t val)
	{
	int idx, old_idx;

	if (!hg)
		return(MX_HISTOGRAM_IDX_UNDEF);

	/* Messwert auf "bucket"-Index abbilden ... */
	idx = mx_histogram_val2idx(hg, val);

	/* ... falls Index-Geschichte existiert ... */
	if (hg->idx_history) {
		/* ... und Zaehlungsbegrenzung erreicht .. */
		old_idx = hg->idx_history[hg->idx_history_top];
		if (old_idx > MX_HISTOGRAM_IDX_UNDEF) {
			/* ... ggf. alte Zaehlung vergessen ... */
			hg->bucket[old_idx]--;
			
			/* ... und Gesamtzaehler korrigieren ... */
			if (old_idx >= 0 && old_idx < hg->n_buckets)
				hg->n_samples--;
			}

		/* ... neuen Index eintragen und Ringpuffer weiterschalten */
		hg->idx_history[hg->idx_history_top] = idx;

		hg->idx_history_top = (hg->idx_history_top + 1)
						% hg->sample_limit;
		}

	/* ... im gewaehlten "bucket" Zaehler erhoehen ... */
	hg->bucket[idx]++;

	/* ... zaehler erhoehen, falls KEIN Ueber-/Unterlauf */
	if (idx >= 0 && idx < hg->n_buckets)
		hg->n_samples++;

	/* ... und IMMER Gesamtzaeher erhoehen */
	hg->tot_samples++;

	return(idx);
	}

int mx_histogram_update_urange(mx_histogram_t *hg,
		mx_real_t min, mx_real_t max, mx_real_t weight)
	{
	int min_idx, max_idx;
	int i, j;

	if (!hg || max < min || weight <= 0)
		return(-1);

	min_idx = mx_histogram_val2idx(hg, min);
	max_idx = mx_histogram_val2idx(hg, max);

	for (i = min_idx; i <= max_idx; i++)
		for (j = 0;
		     j < (int)(weight / (mx_real_t)(max_idx - min_idx + 1));
		     j++)
			mx_histogram_update(hg, mx_histogram_idx2val(hg, i));

	return(0);
	}

int mx_histogram_count(mx_histogram_t *hg,
                mx_real_t val)
	{
	int idx;

	if (!hg) {
		return MX_HISTOGRAM_IDX_UNDEF;
	}

	idx = mx_histogram_val2idx(hg, val);

	return hg->bucket[idx];
	}

mx_real_t mx_histogram_prob(mx_histogram_t *hg, mx_real_t val)
	{
	int idx;

	if (!hg)
		return(0);

	idx = mx_histogram_val2idx(hg, val);

	return(hg->bucket[idx] / hg->n_samples);
	}

mx_real_t mx_histogram_prob4idx(mx_histogram_t *hg, int idx)
	{
	/* Check parameters ... */
	if (!hg)
		return(0.0);

	/* ... and for over and underflow, respectively */
	if (idx < 0)
		return(0.0);
	else if (idx >= hg->n_buckets)
		return(1.0);

	return(hg->bucket[idx] / hg->n_samples);
	}

mx_real_t mx_histogram_prob_le(mx_histogram_t *hg, mx_real_t val)
	{
	int idx;
	mx_real_t idx_low_val, prob;

	/* Messwert in Index umrechnen ... */
	idx = mx_histogram_val2idx(hg, val);

	/* Ueber- und Unterlauf abfangen ... */
	if (idx < 0)
		return(0.0);
	else if (idx >= hg->n_buckets)
		return(1.0);

	/* ... zuerst Wahrscheinlichkeit "unter" 'idx' bestimmen ... */
	prob = mx_histogram_prob_le4idx(hg, idx - 1);

	/* ... dann anteilige Wahrscheinlichkeit fuer "bin" 'idx' ... */
	idx_low_val = mx_histogram_idx2val(hg, idx) - hg->resolution / 2.0;

	prob += (val - idx_low_val) * hg->bucket[idx] /
			(mx_real_t) hg->n_samples;

	return(prob);
	}

mx_real_t mx_histogram_prob_le4idx(mx_histogram_t *hg, int idx)
	{
	int sum_samples;
	int i;
	mx_real_t prob;

	/* Ueber- und Unterlauf abfangen ... */
	if (idx < 0)
		return(0.0);
	else if (idx >= hg->n_buckets)
		return(1.0);

	/* ... Anzahl Samples kleiner gleich 'idx' bestimmen ... */
	for (i = 0, sum_samples = 0; i <= idx; i++)
		sum_samples += hg->bucket[i];

	/* ... und in Wahrscheinlichkeit umrechnen ... */
	prob = (mx_real_t)sum_samples / (mx_real_t) hg->n_samples;

	return(prob);
	}

mx_real_t mx_histogram_prob_ge(mx_histogram_t *hg, mx_real_t val);

mx_real_t mx_histogram_invprob_le(mx_histogram_t *hg, mx_real_t prob)
	{
	mx_real_t n_samples, sum_samples;
	int i, idx;

	/* Wahrscheinlichkeit in # Samples umrechnen ... */
	n_samples = prob * hg->n_samples;
	
	for (i = 0, sum_samples = 0; i < hg->n_buckets; i++) {
		sum_samples += hg->bucket[i];

		if (sum_samples >= n_samples) {
			idx = i;
			break;
			}
		}

	return(hg->min + idx * hg->resolution);
	}

mx_real_t mx_histogram_invprob_ge(mx_histogram_t *hg, mx_real_t prob)
	{
	mx_real_t n_samples, sum_samples;
	int i, idx;

	/* Wahrscheinlichkeit in # Samples umrechnen ... */
	n_samples = prob * hg->n_samples;
	
	for (i = hg->n_buckets - 1, sum_samples = 0; i >= 0; i--) {
		sum_samples += hg->bucket[i];

		if (sum_samples >= n_samples) {
			idx = i;
			break;
			}
		}

	return(hg->min + idx * hg->resolution);
	}

/**
* mx_real_t histogram_mean(*histogram)
*
* DESCRIPTION
*	Calculates the mean of an MX_HISTOGRAM
*	and returns the result as an mx_real_t.
*
*
* PARAMETERS
*	histogram '*histogram' 
*
* 
* RETURN VALUES
*	Returns the calculated mean of the histogram 
* 
**/
mx_real_t mx_histogram_mean(mx_histogram_t *hg) 
	{
	int i;
	mx_real_t mean = 0;

	if (!hg)
		return(0);

	if (hg->n_samples > 0) {
		for (i = 0; i < hg->n_buckets; i++)
			mean += hg->bucket[i] * mx_histogram_idx2val(hg, i);

		mean /= hg->n_samples;
		}

	return(mean);
	}

/**
* mx_real_t histogram_variance(*histogram)
*
* DESCRIPTION
*	Calculates the variance of an MX_HISTOGRAM
*	and returns the result as an mx_real_t.
*
*
* PARAMETERS
*	histogram '*histogram' 
*
* 
* RETURN VALUES
*	Returns the calculated variance of the histogram 
* 
**/
mx_real_t mx_histogram_variance(mx_histogram_t *hg) 
	{
	int i;
	mx_real_t mean;
	mx_real_t variance = 0;
	mx_real_t difference;
	
	if (hg && hg->n_samples > 0) {
		mean = mx_histogram_mean(hg);

		for (i = 0; i < hg->n_buckets; i++) {
			difference = (mx_real_t)mx_histogram_idx2val(hg, i) - mean;
			
			variance += difference * difference * (mx_real_t)hg->bucket[i];
		}
		
		variance /= (mx_real_t)hg->n_samples;
	}
	
	return variance;
	}

/**
* mx_real_t histogram_standard_deviation(*histogram)
*
* DESCRIPTION
*	Calculates the standard deviation of an MX_HISTOGRAM
*	and returns the result as an mx_real_t.
*
*
* PARAMETERS
*	histogram '*histogram' 
*
* 
* RETURN VALUES
*	Returns the calculated standard deviation of the histogram 
* 
**/
mx_real_t mx_histogram_standard_deviation(mx_histogram_t *hg) 
	{
	return sqrt(mx_histogram_variance(hg));
	}

/**
* mx_real_t histogram_mode(*histogram)
*
* DESCRIPTION
*	Calculates the mode of the distribution given by an MX_HISTOGRAM
*	and returns the result as an mx_real_t.
*
* NOTE:	Currently the mode is approximated by returning the range value
*	for which the histogram count reaches its maximum!
*
* PARAMETERS
*	histogram '*histogram' 
*
* 
* RETURN VALUES
*	Returns the calculated mode of the histogram 
* 
**/
mx_real_t mx_histogram_mode(mx_histogram_t *hg) 
	{
	int i;
	mx_real_t idx = 0;
	int max = 0;

	if (!hg)
		return(0);

	for (i = 0; i < hg->n_buckets; i++) {
		if (hg->bucket[i] > max) {
			max = hg->bucket[i];
			idx = i;
			}
		}

	return(mx_histogram_idx2val(hg, idx));
	}

/**
* mx_real_t mx_histogram_otsu_thresh(&m1p, &m2p, *hg);
*
* DESCRIPTION
*	Calculates the optimal binarization threshold for a given histogram
*	according to Otsu's method (i.e. by fitting two normal distributions
*	for "foreground" and "background").
*
*	Additionally, returns the means of the two fitted normal distributions
*	'&m1p' and '&m2p', if the respective pointers are not NULL.
*
* PARAMETERS
*	histogram '*hg'
*
* RETURN VALUES
*	Returns the optimal threshold found.
*
* ERRORS
*	Returns zero if no valid histogram is given.
**/
mx_real_t mx_histogram_otsu_thresh(mx_real_t *m1p, mx_real_t *m2p,
		mx_histogram_t *hg)
	{
	int i;
	mx_real_t q1, q1_neu;
	mx_real_t m1, m2, mean, prob;
	mx_real_t scatter_B, scatter_B_opt;
	mx_real_t thresh = 0.0, m1_opt = 0.0, m2_opt = 0.0;
	
	/* check parameters */
	if (!hg) {
		return 0;
	}
		
	mean = mx_histogram_mean(hg);

	scatter_B_opt = 0.0;
	
	for (i = 0; i < hg->n_buckets; i++) {
		prob = mx_histogram_prob4idx(hg, i);

		if (i == 0) {
			q1 = prob;
			m1 = 0;
			m2 = mean;
		}	else {
			q1_neu = q1 + prob;
			if (q1_neu == 0) {
				m1 = 0;
			} else { 
				m1 = (q1 * m1 + (i) * prob) / q1_neu;
			}
			
			if (q1_neu != 1) {
				m2 = (mean - q1_neu * m1) / (1 - q1_neu);
			}
			
			q1 = q1_neu;
		}
		
		scatter_B = q1 * (1 - q1) * (m1 - m2) * (m1 - m2);
		
		if (scatter_B > scatter_B_opt) {
			scatter_B_opt = scatter_B;

			thresh = mx_histogram_idx2val(hg, i);
			m1_opt = m1;
			m2_opt = m2;
		}
	}

	if (m1p) {
		*m1p = m1_opt;
	}
	
	if (m2p) {
		*m2p = m2_opt;
	}

	return thresh;
	}
