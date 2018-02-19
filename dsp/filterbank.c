/**
* Datei:	filterbank.c
* Autor:	Gernot A. Fink
* Datum:	10.4.1997
*
* Beschreibung:	Trapez-Filterbank zur Anwendung im Frequenzbereich
**/

#include <mx/real.h>

#include <rs/memory.h>

#define DSP_KERNEL
#include "dsp.h"

/**
* dsp_filterbank_create(n_channels, mid_freq[], plateau[])
*	Erzeugt eine Trapez-Filterbank mit 'n_channels' Kanaelen, deren
*	Frequenzgang durch die Mittenfrequenzen 'mid_freq[]' und die
*	Plateaubreiten 'plateau[]' (= Breite des Trapez-Plateaus) bei einer
*	Frequenzaufloesung von 'd_freq' gegeben sind.
*	Zusaetzlich wird der "gueltige" Frequenzbereich im Intervall
*	['f_min'...'f_max'] begrenzt.
*
*	Beachte:	Wird kein Plateaubreitenvektor 'plateau[]' angegeben,
*			so entsteht eine Dreiecksfilterbank.
*
*			Die Bandbreite eines Filterbandes erstreckt sich
*			immer zwischen den Plateaugrenzen der beiden
*			angrenzenden Baender.
*
**/
dsp_filterbank_t *dsp_filterbank_create(int n_channels,
		mx_real_t *mid_freq, mx_real_t *plateau,
		mx_real_t d_freq, mx_real_t f_min, mx_real_t f_max)
	{
	dsp_filterbank_t *fb;
	mx_real_t *ftop, *fbot;
	int *itop, *_itop, *ibot, *_ibot;
	mx_real_t d_freq_2 = d_freq / 2;
	int i, j, ind;
	mx_real_t dc;

	fb = rs_malloc(sizeof(dsp_filterbank_t), "filter bank");

	fb->n_channels = n_channels;
	fb->left_ind = rs_malloc(n_channels * sizeof(int), "filter bank");
	fb->n_inds = rs_malloc(n_channels * sizeof(int), "filter bank");
	fb->weight = rs_malloc(n_channels * sizeof(mx_real_t *), "filter bank");
	ftop = rs_malloc(n_channels * sizeof(mx_real_t), "filter bank");
	fbot = rs_malloc(n_channels * sizeof(mx_real_t), "filter bank");
	_itop = rs_malloc((n_channels + 2) * sizeof(int), "filter bank");
	_ibot = rs_malloc((n_channels + 2) * sizeof(int), "filter bank");
		
	/* damit {ibot|itop}[-1] und {ibot|itop}[n_channels] adressierbar */
	itop = _itop + 1;
	ibot = _ibot + 1;

	/* Breiten und evtl. "Trapezplaetau" der Bandpaesse bestimmen ... */
	for (i = 0; i < n_channels; i++) {
		fbot[i] = mid_freq[i] - ((plateau) ? plateau[i] / 2 : 0);
		ftop[i] = mid_freq[i] + ((plateau) ? plateau[i] / 2 : 0);

		ibot[i] = (fbot[i] + d_freq_2) / d_freq;
		itop[i] = ftop[i] / d_freq;
		}
	itop[-1] = dsp_max(0, ((f_min + d_freq_2) / d_freq) - 1);
	ibot[n_channels] = f_max / d_freq;

	/* ... Speicher fuer Filterkoeffizienten allozieren ... */
	for (i = 0; i < n_channels; i++)
		fb->weight[i] = rs_malloc((ibot[i + 1] - itop[i - 1]) *
						sizeof(mx_real_t),
						"filter bank coefficients");

	/* ... und Koeffizienten berechnen */
	for (i = 0; i < n_channels; i++) {
		ind = 0;
		/* ... ansteigende Flanke ... */
		if (ibot[i] > itop[i - 1]) {
			fb->left_ind[i] = itop[i - 1] + 1;
			dc = 1.0 / (mx_real_t)(ibot[i] - itop[i - 1]);
			for (j = 1; j < ibot[i] - itop[i - 1]; j++)
				fb->weight[i][ind++] = dc * j;
			}
		else	fb->left_ind[i] = ibot[i];
		/* ... Plateau ... */
		for (j = ibot[i]; j <= itop[i]; j++)
			fb->weight[i][ind++] = 1.0;
		/* ... und absteigende Flanke ... */
		if (ibot[i + 1] > itop[i]) {
			dc = 1.0 / (mx_real_t)(ibot[i + 1] - itop[i]);
			for (j = ibot[i + 1] - itop[i] - 1; j > 0; j--)
				fb->weight[i][ind++] = dc * j;
			}
		fb->n_inds[i] = ind;
		}

	free(ftop);
	free(fbot);
	free(_itop);
	free(_ibot);

	return(fb);
	}

/**
* dsp_filterbank_apply()
*	Berechnet die Kanalenergieen 'e[]' mit der Filterbank 'fb' aus dem
*	im Frequenzbereichssignal 'f[]'.
**/
int dsp_filterbank_apply(mx_real_t *e, mx_real_t *f, dsp_filterbank_t *fb)
	{
	int i, j, ch, ind;

	e[0] = 0.0;	/* Gesamtenergie */
	for (i = 0, ch = 1; i < fb->n_channels; i++, ch++) {
		e[ch] = 0.0;	/* Energie im (ch-1)-ten Kanal */
		for (j = 0, ind = fb->left_ind[i];
		     j < fb->n_inds[i];
		     j++, ind++)
			e[ch] += f[ind] * fb->weight[i][j];
		e[0] += e[ch];
		}
	}
