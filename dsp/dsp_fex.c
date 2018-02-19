#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include <rs/messages.h>
#include <rs/io.h>

#include <mx/basics.h>
#include <mx/real.h>

#define DSP_KERNEL
#include "basics.h"
#include "dsp.h"

#define FEX_VERSION	"1.10e"
#define FEX_USAGE	"[<option> ...] <type> <version> [<src> [<dest>]]"
#define FEX_HELP	"\
    where\n\
	<type>	specifies the type of feature extraction to use.\n\
		(Available types are: 'mfcc')\n\
	<version> specifies the feature extraction version to be used.\n\
	<src>	specifies the source signal file; Standard input is used\n\
		if <src> is either omitted or set to '-'.\n\
	<dest>	specifies the destiation feature file. Standard output is\n\
		used if <dest> is either omitted or set to '-'.\n\
    valid options are\n\
	-b	process data in batch mode reading a list of file-name pairs\n\
		holding signal data and feature data, respectively, from\n\
		<src> (or standard input if <src> is omitted or set to '-')\n\
	-c	start with the first frame centered\n\
	-p <param> configure the feature extraction using parameters <param>\n\
	-P	print changed configuration parameters to standard output\n\
    general options are\n\
	-h	display usage information\n\
	-v	be more verbose (can be used multiple times)\n\
	-V	display version information\n\
"

#define FEX_STDIO_NAME	"-"

#define MAX_SAMPLES	1024
#define MAX_FEATURES	256

int fex_batch = 0;
int fex_print_param = 0;
int fex_verbose = 0;

int centered = 0;

int fextract(FILE* dest_fp, dsp_fextract_t *fex, FILE *source_fp);
int next_frame(dsp_sample_t *s, dsp_sample_t *last_s,
		int f_length, int f_shift, FILE *fp);

int main(int argc, char **argv)
	{
	char *type_name, *version_string;
	char *source_name = FEX_STDIO_NAME, *dest_name = FEX_STDIO_NAME;
	char *line;
	char signal_name[_POSIX_PATH_MAX], feature_name[_POSIX_PATH_MAX];
	FILE *source_fp, *dest_fp, *batch_fp;
	int i, j, c, status;
	int tot_frames = 0;
	dsp_fextype_t type;
	int version, version_major, version_minor;
	dsp_fextract_t *fex;
	char *param = NULL;

	 /* Programmnamen bestimmen */
	program = (strrchr(argv[0], '/')) ?
		strrchr(argv[0], '/') + 1 : argv[0];

	/* Optionsbehandlung */
	opterr = 0;     /* KEINE eigene Fehlerbehandlung durch 'getopt' */
	while ((c = getopt(argc, argv, "hVbcp:Pv")) != EOF) {
		switch(c) {
			case 'h':
				fprintf(stderr,
					"usage: %s %s\n%s",
					program, FEX_USAGE, FEX_HELP);
				exit(1);
			case 'V':
				rs_error("version %s (dsp %s, mx %s).",
					FEX_VERSION, dsp_version, mx_version);
			case 'b':
				fex_batch = 1;
				break;
			case 'c':
				centered = 1;
				break;
			case 'p':
				param = optarg;
				break;
			case 'P':
				fex_print_param = 1;
				break;
			case 'v':
				fex_verbose++;
				break;
			case ':':
			case '?':
			default:
				rs_error("incorrect or unknown option '-%c'!",
					optopt);
			}
		}

	/* Argumentuberpruefung */
	if (argc - optind < 2 || argc - optind > 4) {
		/* Hilfe ausgeben, wenn weniger als 2/ mehr als 4 Argumente */
		fprintf(stderr,
			"usage: %s %s\n",
			program, FEX_USAGE);
		exit(1);
		}
	type_name = argv[optind++];
	version_string = argv[optind++];
	if (argc > optind)
		source_name = argv[optind++];
	if (argc > optind)
		dest_name = argv[optind++];

	/* Merkmalsberechnungstyp ... */
	if (strcmp(type_name, "mfcc") == 0)
		type = dsp_fextype_MFCC;
	else	rs_error("illegal feature extraction type '%s'!",
			type_name);

	/* ... und -version bestimmen ... */
	if (sscanf(version_string, "%d.%d", &version_major, &version_minor)
			!= 2)
		rs_error("illegal feature extraction version '%s'!",
			version_string);
		
	version = DSP_MK_VERSION(version_major, version_minor);

	/* ... Merkmalsberechnungdaten erzeugen ... */
	fex = dsp_fextract_create(type, version, param);
	if (!fex)
		rs_error("feature extraction version %d.%d for type '%s' not available!",
			version_major, version_minor, type_name);
	if (fex_verbose > 1)
		dsp_fextract_fprintparam(stderr, fex, "#\t");

	/* Falls kein "Batch"-Betrieb mit Liste von Dateinamen ... */
	if (!fex_batch) {
		/* ... Ein- und Ausgabedatei oeffnen ... */
		if (strcmp(source_name, FEX_STDIO_NAME) == 0)
			source_fp = stdin;
		else	{
			source_fp = fopen(source_name, "r");
			if (!source_fp)
				rs_error("can't read signal data from '%s'!",
					source_name);
			}
		if (strcmp(dest_name, FEX_STDIO_NAME) == 0) {
			if (fex_print_param)
				rs_error("output of parameters would corrupt feature data!");
			dest_fp = stdout;
			}
		else	{
			dest_fp = fopen(dest_name, "w");
			if (!dest_fp)
				rs_error("can't write feature data to '%s'!",
					dest_name);
			}

		/* ... und Merkmale berechnen */
		tot_frames = fextract(dest_fp, fex, source_fp);
		}
	else	{
		/* ... zuerst Datei mit Dateinamensliste oeffnen ... */
		if (strcmp(source_name, FEX_STDIO_NAME) == 0)
			batch_fp = stdin;
		else	{
			batch_fp = fopen(source_name, "r");
			if (!batch_fp)
				rs_error("can't read list of file names from '%s'!",
					source_name);
			}

		/* ... dann fuer alle Zeilen der Eingabe ... */
		while (line = rs_line_read(batch_fp, DSP_COMMENT_CHAR)) {
			/* ... Dateinamen extrahieren ... */
			status = sscanf(line, "%s%s",
					signal_name, feature_name);
			if (status < 1) {
				rs_warning("ignoring ill-formed input line '%s'!", line);
				continue;
				}
			else if (status == 1) {
				if (fex_verbose)
					rs_warning("destination missing for '%s' - results will be discarded.",
						signal_name);
				}

			/* ... Dateien oeffnen ... */
			source_fp = fopen(signal_name, "r");
			if (!source_fp)
				rs_error("can't open '%s'!",
						signal_name);
			if (status == 1)
				dest_fp = NULL;
			else	{
				dest_fp = fopen(feature_name, "w");
				if (!dest_fp)
					rs_error("can't open '%s'!",
						feature_name);
				}

			/* ... und Merkmale berechnen */
			tot_frames += fextract(dest_fp, fex, source_fp);

			/* ... und Dateien wieder schliessen */
			fclose(source_fp);
			if (dest_fp)
				fclose(dest_fp);
			}
		}

	/* ... ggf. veraenderte Berechnungsparameter ausgeben */
	if (fex_print_param)
		dsp_fextract_fprintparam(stdout, fex, "");

	if (fex_verbose)
		rs_msg("%d frames processed.", tot_frames);

	exit(0);
	}

int fextract(FILE* dest_fp, dsp_fextract_t *fex, FILE *source_fp)
	{
	dsp_sample_t s[MAX_SAMPLES];
	dsp_sample_t last_s[MAX_SAMPLES];
	dsp_sample_t *frame, *last_frame, *__tmp;
	mx_real_t f[MAX_FEATURES];

	int frame_len, frame_shift;
	int n_features;
	int n_samples;
	int frames = 0;

	/* Parameter bestimmen ... */
	frame_len =	fex->frame_len * fex->n_channels;
	frame_shift =	fex->frame_shift * fex->n_channels;
	n_features =	fex->n_features;

	/* ersten Frame einlesen ... */
	frame = s; last_frame = last_s;
	n_samples = next_frame(frame, NULL, frame_len, frame_shift, source_fp);

	while (n_samples > 0) {
		frames++;

		n_features = dsp_fextract_calc(fex, f, frame);

		if (dest_fp && n_features > 0) {
			/* ... Daten ausgeben ... */
			fwrite (f, sizeof(mx_real_t), n_features, dest_fp);
			}

		/* ... falls aktueller Frame unvollstaendig, abbrechen! */
		if (n_samples < frame_len)
			break;

		/* ... und naechsten Frame einlesen */
		__tmp = last_frame;
		last_frame = frame;
		frame = __tmp;

		n_samples = next_frame(frame, last_frame,
				frame_len, frame_shift, source_fp);
		}

	return(frames);
	}

/**
* next_frame(s[], last_s[], f_length, f_shift, fp)
*	Liest Abtastwerte fuer den naechsten 'f_length' Samples langen Frame
*	aus der Datei 'fp' in 's[]' ein. Der sich bei einer Fortschaltrate
*	'f_shift', die kleiner als die Frame-Laenge 'f_length' ist,
*	ergebende Ueberlappungsbereich wird dabei aus dem
*	Vorgaenger-Frame 'last_s[]' uebernommen, sofern dieser angegeben ist.
*
*	Liefert die Anzahl der tatsaechlich gelesenen Abtastwerte oder Null,
*	wenn nicht ausreichend, d.h. < 'f_shift' Werte gelesen wurden.
**/
int next_frame(dsp_sample_t *s, dsp_sample_t *last_s,
		int f_length, int f_shift, FILE *fp)
	{
	int i, f_len, n_samples = 0;

	/* Falls ein Vorgaenger-Frame existiert ... */
	if (last_s && f_shift < f_length) {
		/* ... Ueberlappungsbereich kopieren ... */
		memcpy(s, last_s + f_shift,
			(f_length - f_shift) * sizeof(dsp_sample_t));
		n_samples = (f_length - f_shift);
		}
	/* ... sonst, falls Frames "zentriert" werden sollen ... */
	else if (centered) {
		/* ...  Ueberlappungsbereich loeschen ... */
		n_samples = (f_length - f_shift) / 2;
		for (i = 0; i < n_samples; i++)
			s[i] = 0;
		}

	/* ... Frame mit Abtastwerten auffuellen ... */
	n_samples += fread(s + n_samples, sizeof(dsp_sample_t),
			f_length - n_samples, fp);

	/* ... bei nicht ausreichender "Fuellung" -> Ende ... */
	if (n_samples < f_shift)
		return(0);

	/* ... sonst evtl. verbleibenden Bereich loeschen ... */
	for (i = n_samples; i < f_length; i++)
		s[i] = 0;

	/* ... und # tatsaechlich gelesener Abtastwerte zurueckliefern */
	return(n_samples);
	}
