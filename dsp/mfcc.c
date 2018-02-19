/**
* Datei:	mfcc.c
* Autor:	Gernot A. Fink
* Datum:	5.12.1998 (ausgekoppelt aus 'fextract.c)
*
* Beschreibung:	Berechnung von MFCCs (Mel-Frequency-Cepstral-Coefficients)
*		in verschiedenen Versionen
*
*	Versionen 1.x von 'fextract.c':
*		1.1:	in Anlehnung an Erlangen/Ulmer Merkmalsberechnung
*			(T. Kuhn: Die Erkennungsphase in einem Dialogsystem,
*				Bd. 80 von Dissertationen zur kuenstlichen
*				Intelligenz, infix, 1995, S. 228ff.)
*		1.2:	zusaetzlich Kanaladaption durch Bereinigung des
*			cepstralen Mittelwerts und Minimum/Maximum-Normierung
*			des Energieverlaufs
*		1.3:	wie 1.2 aber mit uniformer Kurzzeitnormalisierung
*			des cepstralen Mittelwerts
*
*		1.4:	wie Version 1.3 aber mit Praeemphase, neuer signal-
*			basierter Energieberechnung, histogrammbasierter
*			inkrementeller Schaetzung der Energiedynamik und
*			Normierung des Energiemaximums auf das 95%-Quantil
*			der Verteilung
*		1.5:	reserved
*		1.6:	reserved
*		2.4:	reserved
*		2.5:	reserved
**/

#include <string.h>

#include <rs/memory.h>
#include <rs/messages.h>
#include <rs/io.h>

#define DSP_KERNEL
#include "mfcc.h"
#include "mfcc_1_4.h"

/* Konstantendefinitionen fuer ... */
/* ... Merkmalsberechnung V1.1: */
#define V1_1_SAMPLERATE		16000
#define V1_1_N_CHANNELS		1
#define V1_1_FRAME_LEN		256
#define V1_1_FRAME_SHIFT	160
#define V1_1_N_FRESOLUTION	((mx_real_t)V1_1_SAMPLERATE / V1_1_FRAME_LEN)
#define V1_1_MIN_FREQ		(3 * V1_1_N_FRESOLUTION)
#define V1_1_MAX_FREQ		(V1_1_SAMPLERATE / 2)
#define V1_1_N_FILTERS		31
#define V1_1_W_LENGTH		5
#define V1_1_N_BASEFEATURES	(1 + 12)
#define V1_1_N_FEATURES		(3 * V1_1_N_BASEFEATURES)

static int _dsp_mfcc_1_1(dsp_fextract_t *fex,
		mx_real_t *features, dsp_sample_t *signal);

dsp_fextract_t *dsp_mfcc_create(dsp_fextract_t *fex, char *param)
	{
	dsp_mfcc_t *cfg;
	int i, pos;
	char *cp;

	/* Parameter pruefen ... */
	if (!fex || fex->type != dsp_fextype_MFCC)
		return(NULL);

	/* ... speziellen Datenbereich erzeugen ... */
	cfg = rs_malloc(sizeof(dsp_mfcc_t), "MFCC configuration data");
	fex->config = cfg;

	if (fex->version == DSP_MK_VERSION(1, 4)) {
		_dsp_mfcc_1_4_configure(fex, param);
		return(fex);
		}

	/* Abtastrate etc. ... */
	switch (fex->version) {
		case DSP_MK_VERSION(1, 1):

			fex->samplerate =	V1_1_SAMPLERATE;
			fex->n_channels =	V1_1_N_CHANNELS;
			fex->frame_len =	V1_1_FRAME_LEN;
			fex->frame_shift =	V1_1_FRAME_SHIFT;
			fex->n_features =	V1_1_N_FEATURES;

			/* Verzoegerungselement fuer Ableitung erzeugen ... */
			cfg->wderiv = dsp_delay_create(V1_1_W_LENGTH,
					sizeof(mx_real_t),
					V1_1_N_BASEFEATURES);
			break;

		default:
			return(NULL);
		}

	return(fex);
	}

void dsp_mfcc_destroy(dsp_fextract_t *fex)
	{
	dsp_mfcc_t *cfg;

	/* Parameter pruefen ... */
	if (!fex || fex->type != dsp_fextype_MFCC)
		return;

	/* ... speziellen Datenbereich zugreifbar machen ... */
	cfg = fex->config;

	/* ... und ggf. existierende Eintraege loeschen */
	if (cfg->wderiv)
		/** dsp_delay_destroy(cfg->wderiv) **/ ;
	if (cfg->channel)
		/** dsp_channel_destroy(cfg->channel) **/ ;
	}

void dsp_mfcc_reset(dsp_fextract_t *fex)
	{
	dsp_mfcc_t *cfg;

	/* Parameter pruefen ... */
	if (!fex || fex->type != dsp_fextype_MFCC)
		return;

	/* ... speziellen Datenbereich zugreifbar machen ... */
	cfg = fex->config;

	/* ... und ggf. existierende Datenbestaende re-initialisieren */
	if (cfg->wderiv)
		dsp_delay_flush(cfg->wderiv);
	if (cfg->channel)
		/** kein Reset fuer Kanalmodell **/ ;
	}

int dsp_mfcc_fprintparam(FILE *fp, dsp_fextract_t *fex, char *key)
	{
	dsp_mfcc_t *cfg;

	dsp_channel_t *channel;
	mx_histogram_t *ehist;

	dsp_channel_t *channel_left;
	dsp_channel_t *channel_right;
	mx_histogram_t *ehist_left;
	mx_histogram_t *ehist_right;

	int i, c;

	/* Parameter pruefen ... */
	if (!fex || fex->type != dsp_fextype_MFCC)
		return(-1);

	/* ... speziellen Datenbereich zugreifbar machen ... */
	cfg = fex->config;

	switch (fex->version) {
		case DSP_MK_VERSION(1, 1):
			break;
		case DSP_MK_VERSION(1, 4): 
		       if (!cfg->channel)
			 rs_error("no channel adaptation data present!");
		       channel = cfg->channel;
		       ehist = cfg->ehist;
		       break;
		}

	switch (fex->version) {
		case DSP_MK_VERSION(1, 1):
			break;
		case DSP_MK_VERSION(1, 4): 
			_dsp_mfcc_1_4_fprintparam(fp, fex, key);
			break;
		}

	return(0);
	}

int dsp_mfcc_calc(dsp_fextract_t *fex,
		mx_real_t *features, dsp_sample_t *signal)
	{
	/* Parameter pruefen ... */
	if (!fex || fex->type != dsp_fextype_MFCC)
		return(-1);

	switch (fex->version) {
		case DSP_MK_VERSION(1, 1):
			return (_dsp_mfcc_1_1(fex, features, signal));
		case DSP_MK_VERSION(1, 4):
			return (_dsp_mfcc_1_4(fex, features, signal));
		default:
			return(-1);
		}
	}

	/*
	 * Version 1.1:
	 *	verbesserte/korrigierte Version der ersten
	 *	inkrementelle Merkmalsberechnung auf der Basis der
	 *	Erlangen/Ulmer V3.1N1
	 *
	 *	Frame-Laenge:	256 Samples (bei 16 kHz = 16 ms)
	 *	(Frame-Shift:	160 Samples (bei 16 kHz = 10 ms))
	 *	Mel-Filterbank (inkl. Gesamtenergie in allen Kanaelen)
	 *		NEU: wird automatisch mit 30 Koeff. erzeugt
	 *	KEINE Normierung
	 *	Logarithmus
	 *	Cepstrum (via Cosinustransformation)
	 *	1. und 2. Ableitung (Fenster: 5 Frames)
	 *		KORRIGIERT: C0 etc. ist nicht mehr im Merkmalssatz
	 *	Glaettung der Originaldaten mit "Tirolerhut"
	 */

/**
* _dsp_mfcc_1_1(features, frames)
**/
static int _dsp_mfcc_1_1(dsp_fextract_t *fex,
		mx_real_t *features, dsp_sample_t *signal)
	{
	static mx_real_t mtf[V1_1_N_FILTERS], bbr[V1_1_N_FILTERS];
	static dsp_filterbank_t *fb = NULL;
	static mx_real_t w[V1_1_FRAME_LEN];
	static mx_real_t p[V1_1_FRAME_LEN];
	static mx_complex_t z[V1_1_FRAME_LEN];
	static mx_real_t e[V1_1_N_FILTERS + 1];
	static mx_real_t C[V1_1_N_FILTERS];

	dsp_mfcc_t *cfg = fex->config;
	int i;

	/* ... Filterbank ggf. erzeugen ... */
	if (fb == NULL) {
		/* ... Mel-Skala erzeugen ... */
		if (dsp_mel_create(mtf, bbr, V1_1_N_FRESOLUTION, 
			V1_1_MIN_FREQ, V1_1_MAX_FREQ, 1.0, V1_1_N_FILTERS)
				!= V1_1_N_FILTERS)
			rs_error("problems creating mel-scale!");

		/* ... Plateau ist 1/4 Frequenzgruppe breit ... */
		for (i = 0; i < V1_1_N_FILTERS; i++)
			bbr[i] /= 4;
		fb = dsp_filterbank_create(V1_1_N_FILTERS, mtf, bbr,
				V1_1_N_FRESOLUTION,
				V1_1_MIN_FREQ, V1_1_MAX_FREQ);
		}

	/* Merkmalsberechnung durchfuehren, dazu ... */
	/* ... Hamming-Fenstern ... */
	dsp_window_hamming(w, signal, V1_1_FRAME_LEN);
	
	/* ... Leistungsdichtespektrum ... */
	for (i = 0; i < V1_1_FRAME_LEN; i++) {
		mx_re(z[i]) = w[i];
		mx_im(z[i]) = 0.0;
		}
	dsp_xfft(z, V1_1_FRAME_LEN, 0);
	/* Guenther normiert hinwaerts mit 1/n --- wir nicht */
	for (i = 0; i < V1_1_FRAME_LEN; i++)
		p[i] = (dsp_sqr(mx_re(z[i])) + dsp_sqr(mx_im(z[i]))) /
					dsp_sqr(V1_1_FRAME_LEN);

	/* ... Mel-Filter ... */
	dsp_filterbank_apply(e, p, fb);

	/* ... Logarithmierung ... */
	for (i = 0; i < V1_1_N_FILTERS + 1; i++)
		e[i] = dsp_log10(e[i]);

	/* ... Cepstrum ... */
	dsp_dct(C, e + 1, V1_1_N_FILTERS, V1_1_N_BASEFEATURES);

	/* ... Gesamtmerkmalsvektor erzeugen ... */
	features[0] = e[0];
	memcpy(features + 1, C + 1, sizeof(mx_real_t) * (V1_1_N_BASEFEATURES - 1));

	/* ... Ableitung 1. und 2. Ordnung ... */
	dsp_delay_push(cfg->wderiv, features);

	/* ... berechnen sofern moeglich ... */
	if (dsp_deriv(features + V1_1_N_BASEFEATURES, cfg->wderiv, 1, V1_1_W_LENGTH) == 0 &&
	    dsp_deriv(features + 2 * V1_1_N_BASEFEATURES, cfg->wderiv, 2, V1_1_W_LENGTH) == 0) {
		/* ... zugehoerigen mittleren Vektor getlaettet erzeugen ... */
		dsp_tirol(features, cfg->wderiv);

		/* ... und Daten als gueltig erklaeren ... */
		return(V1_1_N_FEATURES);
		}
	else	return(0);	/* ... sonst: (noch) keine Merkmale! */
	}
