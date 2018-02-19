#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <rs/messages.h>

#include <mx/basics.h>
#include <mx/real.h>
#include <mx/histogram.h>

#define DSP_KERNEL
#include "basics.h"
#include "dsp.h"
#include "vad.h"

#define VAD_VERSION	"1.04c"
#define VAD_USAGE	"[<option> ...] <version> [<src> [<dest>]]"
#define VAD_HELP	"\
    where\n\
	<version> specifies the voice activity detection version to be used.\n\
	<src>	specifies the source signal file; Standard input is used\n\
		if <src> is either omitted or set to '-'.\n\
	<dest>	specifies the destiation signal file. Standard output is\n\
		used if <dest> is either omitted or set to '-'.\n\
    valid options are\n\
	-i	invert activity detection i.e. \"non-activity\" is detected\n\
	-p <file> write voice activity status to protocol <file>\n\
		(if '-' is specified standard output will be used)\n\
	-s [i|s] write short impules 'i' or silence part 's' between\n\
		detected voice activity segments in output\n\
		(default: no separators are inserted)\n\
    general options are\n\
	-h	display usage information\n\
	-v	be more verbose (can be used multiple times)\n\
	-V	print the current program version\n\
"

#define VAD_STDIO_NAME	"-"

#define MAX_SAMPLES	1024

int vad_verbose = 0;

int vad_activity = 1;

dsp_sample_t impulse[] = {0, 0, 32000, 32000, -32000, -32000, 0, 0};
dsp_sample_t silence[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int next_frame(dsp_sample_t *s, dsp_sample_t *last_s,
		int f_length, int f_shift, FILE *fp);

#define VAD_FRAME_LEN	256
#define VAD_FRAME_SHIFT	160

dsp_sample_t *vad_separator = NULL;
int vad_separator_len = 0;

int main(int argc, char **argv)
	{
	char *type_name, *version_string;
	char *source_name = VAD_STDIO_NAME, *dest_name = VAD_STDIO_NAME;
	char *prot_name = NULL;
	FILE *source_fp, *dest_fp;
	FILE *prot_fp = NULL;
	int i, j, c;
	dsp_sample_t s[MAX_SAMPLES];
	dsp_sample_t last_s[MAX_SAMPLES];
	dsp_sample_t *frame, *last_frame, *__tmp;
	dsp_sample_t v[MAX_SAMPLES];
	int n_frames_in, n_frames_out, n_frames_activity, n_samples;
	int frame_len, frame_shift;
	dsp_fextype_t type;
	int version, version_major, version_minor;
	char *param = NULL;
	dsp_vad_t *vad;
	int va, last_va;

	 /* Programmnamen bestimmen */
	program = (strrchr(argv[0], '/')) ?
		strrchr(argv[0], '/') + 1 : argv[0];

	/* Optionsbehandlung */
	opterr = 0;     /* KEINE eigene Fehlerbehandlung durch 'getopt' */
	while ((c = getopt(argc, argv, "hVvip:s:")) != EOF) {
		switch(c) {
			case 'h':
				fprintf(stderr,
					"usage: %s %s\n%s",
					program, VAD_USAGE, VAD_HELP);
				exit(1);
			case 'V':
				rs_error("version %s (dsp %s, mx %s).",
					VAD_VERSION, dsp_version, mx_version);
			case 'i':
				vad_activity = 0;
				break;
			case 'p':
				prot_name = optarg;
				break;
			case 's':
				if (strcmp(optarg, "i") == 0) {
					vad_separator = impulse;
					vad_separator_len =
						sizeof(impulse) /
						sizeof(impulse[0]);
					}
				else if (strcmp(optarg, "i") == 0) {
					vad_separator = silence;
					vad_separator_len =
						sizeof(silence) /
						sizeof(silence[0]);
					}
				else rs_error("illegal vad separator '%s'!",
					optarg);
				break;
			case 'v':
				vad_verbose++;
				break;
			case ':':
			case '?':
			default:
				rs_error("incorrect or unknown option '-%c'!",
					optopt);
			}
		}

	/* Argumentuberpruefung */
	if (argc - optind < 1 || argc - optind > 3) {
		/* Hilfe ausgeben, wenn weniger als 1/ mehr als 3 Argumente */
		fprintf(stderr,
			"usage: %s %s\n",
			program, VAD_USAGE);
		exit(1);
		}
	version_string = argv[optind++];
	if (argc > optind)
		source_name = argv[optind++];
	if (argc > optind)
		dest_name = argv[optind++];

	/* ... und -version bestimmen ... */
	if (sscanf(version_string, "%d.%d", &version_major, &version_minor)
			!= 2)
		rs_error("illegal VAD version '%s'!",
			version_string);
		
	version = DSP_MK_VERSION(version_major, version_minor);

	/* ... VAD-Daten erzeugen ... */
	vad = dsp_vad_create(version, VAD_FRAME_LEN);
	if (!vad)
		rs_error("VAD version %d.%d not available!",
			version_major, version_minor);
	frame_len =	vad->frame_len;
	frame_shift =	VAD_FRAME_SHIFT;

	/* ... Ein- und Ausgabedatei oeffnen ... */
	if (strcmp(source_name, VAD_STDIO_NAME) == 0)
		source_fp = stdin;
	else	{
		source_fp = fopen(source_name, "r");
		if (!source_fp)
			rs_error("can't read signal data from '%s'!",
				source_name);
		}
	if (strcmp(dest_name, VAD_STDIO_NAME) == 0) {
		dest_fp = stdout;
		}
	else	{
		dest_fp = fopen(dest_name, "w");
		if (!dest_fp)
			rs_error("can't write signal data to '%s'!",
				dest_name);
		}

	/* ... ggf. Protokolldatei oeffnen ... */
	if (prot_name) {
		if (strcmp(prot_name, VAD_STDIO_NAME) == 0) {
			if (dest_fp == stdout)
				rs_error("protocol data would corrupt"
					" output signal!");
			prot_fp = stdout;
			}
		else	{
			prot_fp = fopen(prot_name, "w");
			if (!prot_fp)
				rs_error("can't open protocol file '%s'!",
					prot_name);
			}
		}

	/* ersten Frame einlesen ... */
	n_frames_in = n_frames_out = n_frames_activity = 0;
	frame = s; last_frame = last_s;
	n_samples = next_frame(frame, NULL, frame_len, frame_shift, source_fp);

	last_va = -1;	/* d.h. kein vorheriger Aktivitaetstatus bekannt */
	va = -1;	/* d.h. noch kein aktueller Aktivitaetstatus */

	/* ... solange noch Eingabedaten vorliegen ... */
	while (n_samples > 0 || va >= 0) {
		/* ... Voice-Acitivity fuer aktuellen 'frame' bestimmen ... */
		if (n_samples > 0) {
			n_frames_in++;
			va = dsp_vad_calc(v, vad, frame);
			}
		/* ... bzw. gespeicherten Frame abrufen ... */
		else	{
			va = dsp_vad_calc(v, vad, NULL);
			}

		/* ... falls Aktivitaetswechsel im Signal(!) vorliegt ... */
		if (va >= 0 && va != last_va && n_frames_out > 0) {
#ifdef PROTOCOL_FRAMEWISE
			if (prot_fp)
				fprintf(prot_fp, "---------\n");
#endif /* PROTOCOL_FRAMEWISE */

			/* ... bei negativer Flanke Trennsignal ausgeben ... */
			if (va == (1 - vad_activity) && vad_separator)
				fwrite(vad_separator, sizeof(dsp_sample_t),
					vad_separator_len, dest_fp);

			if (prot_fp) {
				if (va == vad_activity)
					fprintf(prot_fp, "[%d..",
						n_frames_out + 1);
				else	fprintf(prot_fp, "%d]\n",
						n_frames_out);
				}
			}

		/* ... letzten gueltigen(!) Aktivitaetstatus speichern ... */
		if (va >= 0)
			last_va = va;

		/* ... falls Aktivitaet, aktuellen 'frame' ausgeben ... */
		if (va == vad_activity) {
			n_frames_out++;		/* gueliger Ausgabeframe! */
			n_frames_activity++;	/* Frame mit Aktivitaet */

			fwrite(v, sizeof(dsp_sample_t), frame_shift,
				dest_fp);

#ifdef PROTOCOL_FRAMEWISE
			if (prot_fp)
				fprintf(prot_fp, "%5d: %d\n", n_frames_out, va);
#endif /* PROTOCOL_FRAMEWISE */
			}
		/* ... sofern Entscheidung(!) fuer keine Aktivitaet ... */
		while (va == (1 - vad_activity)) {
			n_frames_out++;		/* gueliger Ausgabeframe! */

			/* ... ggf. gepufferte Daten loeschen/ueberspringen */
			va = dsp_vad_calc(v, vad, NULL);

#ifdef PROTOCOL_FRAMEWISE
			if (prot_fp)
				fprintf(prot_fp, "%5d: %d\n", n_frames_out, va);
#endif /* PROTOCOL_FRAMEWISE */
			}

		/* ... und naechsten Frame einlesen */
		__tmp = last_frame;
		last_frame = frame;
		frame = __tmp;
		n_samples = next_frame(frame, last_frame,
				frame_len, frame_shift, source_fp);
		}

	/* ... ggf. letztes aktives Segment beenden ... */
	if (last_va == vad_activity && n_frames_out > 0 && prot_fp) {
		fprintf(prot_fp, "%d]\n",
			n_frames_out);
		}

#ifdef RECALL_STORED_FRAMES_AT_END
	/* ... ggf. gespeicherte Frames abrufen ... */
	va = dsp_vad_calc(v, vad, NULL);
	while (va == vad_activity) {
		n_frames_out++;		/* gueliger Ausgabeframe! */

		fwrite(v, sizeof(dsp_sample_t), frame_shift, dest_fp);

#ifdef PROTOCOL_FRAMEWISE
		if (prot_fp)
			fprintf(prot_fp, "%5d: %d\n", n_frames_out, va);
#endif /* PROTOCOL_FRAMEWISE */

		/* ... ggf. weitere gepufferte Daten abfragen */
		va = dsp_vad_calc(v, vad, NULL);
		}
#endif /* RECALL_STORED_FRAMES_AT_END */

	if (vad_verbose) {
		if (n_frames_in <= 0)
			rs_msg("no input frames processed.");
		else if (n_frames_activity <= 0)
			rs_msg("no activity frames "
				"extracted from %d input frames.",
				n_frames_in);
		else	rs_msg("%d activity frames (%g%%) "
				"extracted from %d input frames.",
			n_frames_activity,
			100.0 * (float)n_frames_activity / (float)n_frames_in,
			n_frames_in);
		}
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
