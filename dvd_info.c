#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include "dvd_info.h"
#include "dvd_device.h"
#include "dvd_vmg_ifo.h"
#include "dvd_vts.h"
#include "dvd_track.h"
#include "dvd_chapter.h"
#include "dvd_cell.h"
#include "dvd_video.h"
#include "dvd_audio.h"
#include "dvd_subtitles.h"
#include "dvd_time.h"
#include "dvd_json.h"
#include "dvd_xchap.h"
#include "dvd_vob.h"
#ifdef __linux__
#include <linux/cdrom.h>
#include "dvd_drive.h"
#endif
#ifndef DVD_INFO_VERSION
#define DVD_INFO_VERSION "1.7_beta1"
#endif

int main(int argc, char **argv) {

	// Program name
	bool p_dvd_json = false;
	bool p_dvd_xchap = false;

	// lsdvd similar display output
	bool d_audio = false;
	bool d_video = false;
	bool d_chapters = false;
	bool d_subtitles = false;
	bool d_cells = false;

	// How much output
	bool verbose = false;
	bool debug = false;

	// limit results
	bool d_has_audio = false;
	bool d_has_subtitles = false;
	bool opt_min_seconds = true;
	unsigned long int arg_number = 0;
	uint32_t arg_min_seconds = 0;
	bool opt_min_minutes = true;
	uint32_t arg_min_minutes = 0;
	bool opt_vts = false;
	uint16_t arg_vts = 0;

	// dvd_info
	char dvdread_id[DVD_DVDREAD_ID + 1] = {'\0'};
	bool d_disc_title_header = true;
	uint16_t d_first_track = 1;
	uint16_t d_last_track = 1;
	uint16_t track_number = 1;
	uint16_t vts = 1;
	uint8_t audio_track_ix = 0;
	uint8_t subtitle_track_ix = 0;
	uint8_t chapter_ix = 0;
	uint8_t cell_ix = 0;
	uint8_t d_stream_num = 0;

	// Device hardware
	int dvd_fd = 0;
	const char *device_filename = NULL;

	// libdvdread
	dvd_reader_t *dvdread_dvd = NULL;
	ifo_handle_t *vmg_ifo = NULL;
	ifo_handle_t *vts_ifo = NULL;

	// DVD
	struct dvd_info dvd_info;
	memset(dvd_info.dvdread_id, '\0', sizeof(dvd_info.dvdread_id));
	dvd_info.video_title_sets = 1;
	dvd_info.side = 1;
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	memset(dvd_info.provider_id, '\0', sizeof(dvd_info.provider_id));
	memset(dvd_info.vmg_id, '\0', sizeof(dvd_info.vmg_id));
	dvd_info.tracks = 1;
	dvd_info.longest_track = 1;
	dvd_info.valid_video_title_sets = 0;
	dvd_info.invalid_video_title_sets = 0;
	dvd_info.valid_tracks = 0;
	dvd_info.invalid_tracks = 0;

	// Video Title Set
	struct dvd_vts dvd_vts[99];

	// Track
	struct dvd_track dvd_track;
	dvd_track.track = 1;
	dvd_track.valid = true;
	dvd_track.vts = 1;
	dvd_track.ttn = 1;
	snprintf(dvd_track.length, DVD_TRACK_LENGTH + 1, "00:00:00.000");
	dvd_track.msecs = 0;
	dvd_track.chapters = 0;
	dvd_track.audio_tracks = 0;
	dvd_track.active_audio_streams = 0;
	dvd_track.subtitles = 0;
	dvd_track.active_subs = 0;
	dvd_track.cells = 0;

	// Video
	struct dvd_video dvd_video;
	memset(dvd_video.codec, '\0', sizeof(dvd_video.codec));
	memset(dvd_video.format, '\0', sizeof(dvd_video.format));
	memset(dvd_video.aspect_ratio, '\0', sizeof(dvd_video.aspect_ratio));
	dvd_video.width = 0;
	dvd_video.height = 0;
	dvd_video.letterbox = false;
	dvd_video.pan_and_scan = false;
	dvd_video.df = 3;
	memset(dvd_video.fps, '\0', sizeof(dvd_video.fps));
	dvd_video.angles = 1;

	// Audio
	struct dvd_audio dvd_audio;
	dvd_audio.ix = 0;
	dvd_audio.track = 1;
	dvd_audio.active = false;
	memset(dvd_audio.stream_id, '\0', sizeof(dvd_audio.stream_id));
	memset(dvd_audio.lang_code, '\0', sizeof(dvd_audio.lang_code));
	memset(dvd_audio.codec, '\0', sizeof(dvd_audio.codec));
	dvd_audio.channels = 0;

	// Subtitles
	struct dvd_subtitle dvd_subtitle;
	dvd_subtitle.track = 1;
	dvd_subtitle.active = false;
	memset(dvd_subtitle.stream_id, '\0', sizeof(dvd_subtitle.stream_id));
	memset(dvd_subtitle.lang_code, '\0', sizeof(dvd_subtitle.lang_code));

	// Chapters
	struct dvd_chapter dvd_chapter;
	dvd_chapter.chapter = 0;
	snprintf(dvd_chapter.length, DVD_CHAPTER_LENGTH + 1, "00:00:00.000");
	dvd_chapter.first_cell = 1;
	dvd_chapter.last_cell = 1;

	// Cells
	struct dvd_cell dvd_cell;
	dvd_cell.cell = 1;
	memset(dvd_cell.length, '\0', sizeof(dvd_cell.length));
	snprintf(dvd_cell.length, DVD_CELL_LENGTH + 1, "00:00:00.000");
	dvd_cell.msecs = 0;
	dvd_cell.first_sector = 0;
	dvd_cell.last_sector = 0;
	dvd_cell.filesize = 0;
	dvd_cell.filesize_mbs = 0;

	// Display formats
	const char *display_formats[4] = { "Pan and Scan or Letterbox", "Pan and Scan", "Letterbox", "Unset" };

	// Statistics
	uint32_t longest_msecs = 0;

	// getopt_long
	bool valid_args = true;
	bool opt_track_number = false;
	uint16_t arg_track_number = 0;
	int ix = 0;
	int opt = 0;
	bool invalid_opt = false;
	const char p_short_opts[] = "aAcdeE:ghjM:sST:t:Vvxz";

	struct option p_long_opts[] = {

		{ "audio", no_argument, NULL, 'a' },
		{ "has-audio", no_argument, NULL, 'A' },
		{ "video", no_argument, NULL, 'v' },
		{ "chapters", no_argument, NULL, 'c' },
		{ "subtitles", no_argument, NULL, 's' },
		{ "has-subtitles", no_argument, NULL, 'S' },
		{ "cells", no_argument, NULL, 'd' },
		{ "all", no_argument, NULL, 'x' },
		{ "json", no_argument, NULL, 'j' },
		{ "track", required_argument, NULL, 't' },
		{ "xchap", no_argument, NULL, 'g' },
		{ "min-seconds", required_argument, NULL, 'E' },
		{ "min-minutes", required_argument, NULL, 'M' },
		{ "vts", required_argument, NULL, 'T' },
		{ "help", no_argument, NULL, 'h' },
		{ "verbose", no_argument, NULL, 'e' },
		{ "version", no_argument, NULL, 'V' },
		{ "debug", no_argument, NULL, 'z' },
		{ 0, 0, 0, 0 }

	};

	// parse options
	while((opt = getopt_long(argc, argv, p_short_opts, p_long_opts, &ix)) != -1) {

		// It's worth noting that if there are unknown options passed,
		// I just ignore them, and continue printing requested data.
		switch(opt) {

			case 'a':
				d_audio = true;
				break;

			case 'A':
				d_has_audio = true;
				break;

			case 'c':
				d_chapters = true;
				break;

			case 'd':
				d_cells = true;
				break;

			case 'e':
				verbose = true;
				break;

			case 'E':
				opt_min_seconds = true;
				arg_min_seconds = (uint32_t)strtoul(optarg, NULL, 10);
				break;

			case 'g':
				p_dvd_xchap = true;
				d_disc_title_header = false;
				break;

			case 'j':
				p_dvd_json = true;
				d_disc_title_header = false;
				break;

			case 'M':
				opt_min_minutes = true;
				arg_min_minutes = (uint32_t)strtoul(optarg, NULL, 10);
				break;

			case 's':
				d_subtitles = true;
				break;

			case 'S':
				d_has_subtitles = true;
				break;

			case 'T':
				opt_vts = true;
				arg_number = strtoul(optarg, NULL, 10);
				if(arg_number > 99)
					arg_vts = 99;
				else
					arg_vts = (uint16_t)arg_number;
				break;


			case 't':
				opt_track_number = true;
				arg_number = strtoul(optarg, NULL, 10);
				if(arg_number > 99)
					arg_track_number = 99;
				else if(arg_number == 0)
					arg_track_number = 1;
				else
					arg_track_number = (uint16_t)arg_number;
				break;

			case 'v':
				d_video = true;
				break;

			case 'V':
				printf("dvd_info %s\n", DVD_INFO_VERSION);
				return 0;

			case 'x':
				d_audio = true;
				d_video = true;
				d_chapters = true;
				d_subtitles = true;
				d_cells = true;
				break;

			case 'z':
				debug = true;
				break;

			// ignore unknown arguments
			case '?':
				invalid_opt = true;
			case 'h':
				printf("dvd_info %s - display information about a DVD\n", DVD_INFO_VERSION);
				printf("\n");
				printf("Usage: dvd_info [path] [options]\n");
				printf("\n");
				printf("Options:\n");
				printf("  -t, --track <number>  Limit to selected track (default: all tracks)\n");
				printf("  -j, --json            Display output in JSON format\n");
				printf("\n");
				printf("Detailed information:\n");
				printf("  -v, --video           Display video streams\n");
				printf("  -a, --audio           Display audio streams\n");
				printf("  -s, --subtitles       Display VobSub subtitles\n");
				printf("  -c, --chapters        Display chapters\n");
				printf("  -d, --cells           Display cells\n");
				printf("  -x, --all             Display all\n");
				printf("\n");
				printf("Narrow results:\n");
				printf("  -A, --has-audio       Track has audio streams\n");
				printf("  -S, --has-subtitles   Track has VobSub subtitles\n");
				printf("  -E, --seconds <secs>  Track has minimum number of seconds\n");
				printf("  -M, --minutes <mins>  Track has minimum number of minutes\n");
				printf("  -T, --vts <number>    Track is in video title set number\n");
				printf("\n");
				printf("Other:\n");
				printf("  -g, --xchap           Display title's chapter format for mkvmerge\n");
				printf("  -e, --verbose         Display invalid tracks and inactive streams\n");
				printf("  -h, --help            Display these help options\n");
				printf("      --version         Version information\n");
				printf("\n");
				printf("DVD path can be a device name, a single file, or a directory (default: %s)\n", DEFAULT_DVD_DEVICE);
				if(invalid_opt)
					return 1;
				return 0;
			// let getopt_long set the variable
			case 0:
			default:
				break;

		}

	}

	if(debug)
		verbose = true;

	// If '-i /dev/device' is not passed, then set it to the string
	// passed.  fex: 'dvd_info /dev/dvd1' would change it from the default
	// of '/dev/dvd'.
	if (argv[optind])
		device_filename = argv[optind];
	else
		device_filename = DEFAULT_DVD_DEVICE;

	// Exit after all invalid input warnings have been sent
	if(valid_args == false)
		return 1;

	/** Begin dvd_info :) */

	// Check to see if device can be accessed
	if(!dvd_device_access(device_filename)) {
		fprintf(stderr, "Cannot access %s\n", device_filename);
		return 1;
	}

	// Check to see if device can be opened
	dvd_fd = dvd_device_open(device_filename);
	if(dvd_fd < 0) {
		fprintf(stderr, "Could not open %s\n", device_filename);
		return 1;
	}
	dvd_device_close(dvd_fd);

#ifdef __linux__

	// Poll drive status if it is hardware
	if(dvd_device_is_hardware(device_filename)) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "DVD drive status: ");
			dvd_drive_display_status(device_filename);

			return 1;

		}

	}

#endif

	// begin libdvdread usage

	// Open DVD device
	dvdread_dvd = DVDOpen(device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "Opening DVD %s failed\n", device_filename);
		return 1;
	}

	// Check if DVD has an identifier, fail otherwise
	dvd_dvdread_id(dvdread_id, dvdread_dvd);
	if(strlen(dvdread_id) == 0) {
		fprintf(stderr, "Opening DVD %s failed\n", device_filename);
		return 1;
	}

	// Open VMG IFO -- where all the cool stuff is
	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "Opening VMG IFO failed\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Get the total number of title tracks on the DVD
	dvd_info.tracks = dvd_tracks(vmg_ifo);

	// Exit if track number requested does not exist
	if(opt_track_number && (arg_track_number > dvd_info.tracks || arg_track_number < 1)) {
		fprintf(stderr, "Valid track numbers: 1 to %" PRIu16 "\n", dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	} else if(opt_track_number) {
		d_first_track = arg_track_number;
		d_last_track = arg_track_number;
		track_number = d_first_track;
	} else {
		d_first_track = 1;
		d_last_track = dvd_info.tracks;
	}

	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	ifo_handle_t *vts_ifos[DVD_MAX_VTS_IFOS];

	if(opt_vts && (arg_vts == 0 || arg_vts > dvd_info.video_title_sets)) {
		fprintf(stderr, "Video Title Set must be between 1 and %" PRIu16 "\n", dvd_info.video_title_sets);
		return 1;
	}

	uint8_t vts_ifo_ix;
	for(vts_ifo_ix = 0; vts_ifo_ix < 100; vts_ifo_ix++)
		vts_ifos[vts_ifo_ix] = NULL;

	// Do some checks to see if a VTS is ok or not
	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		dvd_vts[vts].vts = vts;
		dvd_vts[vts].valid = false;
		dvd_vts[vts].blocks = 0;
		dvd_vts[vts].filesize = 0;
		dvd_vts[vts].vobs = 0;
		dvd_vts[vts].tracks = 0;
		dvd_vts[vts].valid_tracks = 0;
		dvd_vts[vts].invalid_tracks = 0;

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(vts_ifos[vts] == NULL) {
			dvd_vts[vts].valid = false;
			continue;
		}

		if(!ifo_is_vts(vts_ifos[vts])) {
			dvd_vts[vts].valid = false;
			ifoClose(vts_ifos[vts]);
			vts_ifos[vts] = NULL;
			continue;
		}

		dvd_vts[vts].filesize = dvd_vts_filesize(dvdread_dvd, vts);
		if(!dvd_vts[vts].filesize) {
			dvd_vts[vts].valid = false;
			continue;
		}

		dvd_vts[vts].valid = true;

	}

	// GRAB ALL THE THINGS
	dvd_title(dvd_info.title, device_filename);
	dvd_info.side = dvd_info_side(vmg_ifo);
	dvd_provider_id(dvd_info.provider_id, vmg_ifo);
	dvd_vmg_id(dvd_info.vmg_id, vmg_ifo);
	strncpy(dvd_info.dvdread_id, dvdread_id, DVD_DVDREAD_ID);

	/**
	 * Track information
	 */

	struct dvd_track dvd_tracks[DVD_MAX_TRACKS];

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		dvd_track.track = track_number;

		// Initialize track to default values
		dvd_track.valid = true;
		snprintf(dvd_track.length, DVD_TRACK_LENGTH + 1, "00:00:00.000");
		dvd_track.msecs = 0;
		dvd_track.chapters = 0;
		dvd_track.audio_tracks = 0;
		dvd_track.active_audio_streams = 0;
		dvd_track.subtitles = 0;
		dvd_track.active_subs = 0;
		dvd_track.cells = 0;
		dvd_track.filesize_mbs = 0;

		memset(dvd_video.codec, '\0', sizeof(dvd_video.codec));
		memset(dvd_video.format, '\0', sizeof(dvd_video.format));
		memset(dvd_video.aspect_ratio, '\0', sizeof(dvd_video.aspect_ratio));
		memset(dvd_video.fps, '\0', sizeof(dvd_video.fps));
		dvd_video.width = 0;
		dvd_video.height = 0;
		dvd_video.letterbox = false;
		dvd_video.pan_and_scan = false;
		dvd_video.df = 3;
		dvd_video.angles = 0;
		dvd_track.dvd_video = dvd_video;

		// There are two ways a track can be marked as invalid - either the VTS
		// is bad, or the track has an empty length. The first one, it could be
		// a number of things, but the second is likely by design in order to
		// break DVD software. Invalid DVD tracks appear as completely empty in
		// dvd_info's output.

		// If the IFO is invalid, skip the residing track
		// These are hard to find, so an example DVD is '4b4d78c077ea78576a7de09aee7715d4'
		dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, track_number);
		if(dvd_vts[dvd_track.vts].valid == false) {
			dvd_track.valid = false;
			dvd_tracks[track_number - 1] = dvd_track;
			dvd_vts[dvd_track.vts].invalid_tracks++;
			dvd_info.invalid_tracks++;
			continue;
		}

		dvd_track.ttn = dvd_track_ttn(vmg_ifo, dvd_track.track);
		dvd_vts[dvd_track.vts].tracks++;
		vts_ifo = vts_ifos[dvd_track.vts];

		// If the length is empty, disregard all other data attached to it, and mark as invalid
		dvd_track.msecs = dvd_track_msecs(vmg_ifo, vts_ifo, dvd_track.track);

		if(dvd_track.msecs == 0) {
			dvd_track.valid = false;
			dvd_vts[dvd_track.vts].invalid_tracks++;
			dvd_tracks[track_number - 1] = dvd_track;
			dvd_info.invalid_tracks++;
			continue;
		}

		dvd_track_length(dvd_track.length, vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.chapters = dvd_track_chapters(vmg_ifo, vts_ifo, dvd_track.track);

		dvd_vts[dvd_track.vts].valid_tracks++;
		dvd_info.valid_tracks++;

		if(dvd_track.msecs > longest_msecs) {
			dvd_info.longest_track = dvd_track.track;
			longest_msecs = dvd_track.msecs;
		}

		/** Video **/

		dvd_video_codec(dvd_video.codec, vts_ifo);
		dvd_track_video_format(dvd_video.format, vts_ifo);
		dvd_video.width = dvd_video_width(vts_ifo);
		dvd_video.height = dvd_video_height(vts_ifo);
		dvd_video_aspect_ratio(dvd_video.aspect_ratio, vts_ifo);
		dvd_video.letterbox = dvd_video_letterbox(vts_ifo);
		dvd_video.pan_and_scan = dvd_video_pan_scan(vts_ifo);
		dvd_video.df = dvd_video_df(vts_ifo);
		dvd_video.angles = dvd_video_angles(vmg_ifo, dvd_track.track);
		dvd_track.audio_tracks = dvd_track_audio_tracks(vts_ifo);
		dvd_track.active_audio_streams = 0;
		dvd_track.subtitles = dvd_track_subtitles(vts_ifo);
		dvd_track.active_subs = 0;
		dvd_track.cells = dvd_track_cells(vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.filesize_mbs = dvd_track_filesize_mbs(vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track_str_fps(dvd_video.fps, vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.dvd_video = dvd_video;

		/** Audio Streams **/

		dvd_track.dvd_audio_tracks = calloc(dvd_track.audio_tracks, sizeof(*dvd_track.dvd_audio_tracks));

		if(dvd_track.audio_tracks && dvd_track.dvd_audio_tracks != NULL) {

			for(audio_track_ix = 0; audio_track_ix < dvd_track.audio_tracks; audio_track_ix++) {

				memset(&dvd_audio, 0, sizeof(dvd_audio));

				dvd_audio.track = audio_track_ix + 1;

				dvd_audio.active = dvd_audio_active(vmg_ifo, vts_ifo, dvd_track.track, audio_track_ix);
				if(dvd_audio.active)
					dvd_track.active_audio_streams++;

				dvd_audio.channels = dvd_audio_channels(vts_ifo, audio_track_ix);

				memset(dvd_audio.stream_id, '\0', sizeof(dvd_audio.stream_id));
				dvd_audio_stream_id(dvd_audio.stream_id, vts_ifo, audio_track_ix);

				memset(dvd_audio.lang_code, '\0', sizeof(dvd_audio.lang_code));
				dvd_audio_lang_code(dvd_audio.lang_code, vts_ifo, audio_track_ix);

				memset(dvd_audio.codec, '\0', sizeof(dvd_audio.codec));
				dvd_audio_codec(dvd_audio.codec, vts_ifo, audio_track_ix);

				dvd_track.dvd_audio_tracks[audio_track_ix] = dvd_audio;

			}

		}

		/** Subtitles **/

		dvd_track.dvd_subtitles = calloc(dvd_track.subtitles, sizeof(*dvd_track.dvd_subtitles));

		if(dvd_track.subtitles && dvd_track.dvd_subtitles != NULL) {

			for(subtitle_track_ix = 0; subtitle_track_ix < dvd_track.subtitles; subtitle_track_ix++) {

				memset(&dvd_subtitle, 0, sizeof(dvd_subtitle));
				memset(dvd_subtitle.lang_code, '\0', sizeof(dvd_subtitle.lang_code));

				dvd_subtitle.track = subtitle_track_ix + 1;
				dvd_subtitle.active = dvd_subtitle_active(vmg_ifo, vts_ifo, dvd_track.track, dvd_subtitle.track);
				if(dvd_subtitle.active)
					dvd_track.active_subs++;

				memset(dvd_subtitle.stream_id, 0, sizeof(dvd_subtitle.stream_id));
				dvd_subtitle_stream_id(dvd_subtitle.stream_id, subtitle_track_ix);

				memset(dvd_subtitle.lang_code, 0, sizeof(dvd_subtitle.lang_code));
				dvd_subtitle_lang_code(dvd_subtitle.lang_code, vts_ifo, subtitle_track_ix);

				dvd_track.dvd_subtitles[subtitle_track_ix] = dvd_subtitle;

			}

		}

		/** Chapters **/

		dvd_track.dvd_chapters = calloc(dvd_track.chapters, sizeof(*dvd_track.dvd_chapters));

		if(dvd_track.chapters && dvd_track.dvd_chapters != NULL) {

			for(chapter_ix = 0; chapter_ix < dvd_track.chapters; chapter_ix++) {

				dvd_chapter.chapter = chapter_ix + 1;

				snprintf(dvd_chapter.length, DVD_CHAPTER_LENGTH + 1, "00:00:00.000");
				dvd_chapter_length(dvd_chapter.length, vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);

				dvd_chapter.msecs = dvd_chapter_msecs(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
				dvd_chapter.first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
				dvd_chapter.last_cell = dvd_chapter_last_cell(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);

				dvd_track.dvd_chapters[chapter_ix] = dvd_chapter;

			};

		}

		/** Cells **/

		dvd_track.dvd_cells = calloc(dvd_track.cells, sizeof(*dvd_track.dvd_cells));

		if(dvd_track.cells && dvd_track.dvd_cells != NULL) {

			for(cell_ix = 0; cell_ix < dvd_track.cells; cell_ix++) {

				dvd_cell.cell = cell_ix + 1;

				snprintf(dvd_cell.length, DVD_CELL_LENGTH + 1, "00:00:00.000");
				dvd_cell_length(dvd_cell.length, vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);

				dvd_cell.msecs = dvd_cell_msecs(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
				dvd_cell.first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
				dvd_cell.last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
				dvd_cell.blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
				dvd_cell.filesize = dvd_cell_filesize(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
				dvd_cell.filesize_mbs = dvd_cell_filesize_mbs(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);

				dvd_track.dvd_cells[cell_ix] = dvd_cell;

			}

		}

		dvd_tracks[track_number - 1] = dvd_track;

	}

	/** JSON display output **/

	if(p_dvd_json) {
		dvd_json(dvd_info, dvd_tracks, track_number, d_first_track, d_last_track);
		goto cleanup;
	}

	// Start dvd_info output

	if(opt_track_number)
		d_disc_title_header = false;

	if(d_disc_title_header && !p_dvd_xchap) {
		printf("Disc title: '%s', ", dvd_info.title);
		printf("ID: '%s', ", dvd_info.dvdread_id);
		printf("VTSs: %" PRIu16", ", dvd_info.video_title_sets);
		printf("Total tracks: %" PRIu16 ", ", dvd_info.tracks);
		printf("Valid: %" PRIu16 ", ", dvd_info.valid_tracks);
		printf("Longest: %" PRIu16, dvd_info.longest_track);
		printf("\n");
	}

	/** dvdxchap display output **/
	if(p_dvd_xchap) {
		if(opt_track_number)
			dvd_xchap(dvd_tracks[arg_track_number - 1]);
		else
			dvd_xchap(dvd_tracks[dvd_info.longest_track - 1]);
		goto cleanup;
	}

	/**
	 * lsdvd style output (default)
	 *
	 * Note that there are some differences between the JSON output and
	 * the lsdvd one:
	 *
	 * - lsdvd output only displays *active* audio tracks, while the JSON
	 *   shows all of them, but they are flagged as active or not.
	 */

	// Print the valid and invalid VTSs
	if(debug) {

		printf("        Tracks: %02" PRIu16 ", ", dvd_info.tracks);
		printf("Valid: %02" PRIu16 ", ", dvd_info.valid_tracks);
		printf("Invalid: %02" PRIu16, dvd_info.invalid_tracks);
		printf("\n");

		for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {
			if(dvd_vts[vts].valid == true)
				dvd_info.valid_video_title_sets++;
			else
				dvd_info.invalid_video_title_sets++;
		}

		printf("        Video Title Sets: %02" PRIu16 ", ", dvd_info.video_title_sets);
		printf("Valid: %02" PRIu16 ", ", dvd_info.valid_video_title_sets);
		printf("Invalid: %02" PRIu16, dvd_info.invalid_video_title_sets);
		printf("\n");

		for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {
			printf("        VTS: %02" PRIu16 ", ", vts);
			printf("Tracks: %02" PRIu16 ", ", dvd_vts[vts].tracks);
			printf("Valid: %02" PRIu16 ", ", dvd_vts[vts].valid_tracks);
			printf("Invalid: %02" PRIu16, dvd_vts[vts].invalid_tracks);
			printf("\n");
		}

	}

	// Display more specific Video Title Set information
	if(opt_vts) {
		printf("        Video Title Set: %02" PRIu16 ", ", arg_vts);
		printf("Tracks: %02" PRIu16 ", ", dvd_vts[arg_vts].tracks);
		printf("Valid tracks: %02" PRIu16 ", ", dvd_vts[arg_vts].valid_tracks);
		printf("Invalid tracks: %02" PRIu16, dvd_vts[arg_vts].invalid_tracks);
		printf("\n");
	}

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		// Skip invalid tracks
		if(dvd_tracks[track_number - 1].valid == false && !p_dvd_json && !verbose && !opt_vts && !(opt_track_number && track_number == arg_track_number))
			continue;

		dvd_track = dvd_tracks[track_number - 1];
		dvd_video = dvd_tracks[track_number - 1].dvd_video;

		// Skip tracks less than a second long
		if((dvd_track.msecs < 1000) && !verbose && !(opt_track_number && track_number == arg_track_number))
			continue;

		// Skip if limiting to tracks with audio only
		if(d_has_audio && dvd_track.active_audio_streams == 0)
			continue;

		// Skip if limiting tracks to one with VOBSUB subtitles only (cc not supported)
		if(d_has_subtitles && dvd_track.active_subs == 0)
			continue;

		// Skip if limiting to a minimum # of seconds which the length doesn't meet
		if(opt_min_seconds && dvd_track.msecs < (arg_min_seconds * 1000))
			continue;

		// Skip if limiting to a minimum # of minutes which the length doesn't meet
		if(opt_min_minutes && dvd_track.msecs < (arg_min_minutes * 1000 * 60))
			continue;

		// Skip if limiting to one title set
		if(opt_vts && dvd_track.vts != arg_vts)
			continue;

		// Display track information
		printf("Track: %02" PRIu16 ", ", dvd_track.track);
		printf("Length: %s, ", dvd_track.length);
		printf("Chapters: %02" PRIu8 ", ", dvd_track.chapters);
		printf("Cells: %02" PRIu8 ", ", dvd_track.cells);
		printf("Audio streams: %02" PRIu8 ", ", dvd_track.active_audio_streams);
		printf("Subpictures: %02" PRIu8 ", ", dvd_track.active_subs);
		if(debug || opt_vts) {
			printf("VTS: %02" PRIu16", ", dvd_track.vts);
			printf("Valid VTS: %s, ", dvd_vts[dvd_track.vts].valid ? "yes" : " no");
		}
		if(verbose)
			printf("Valid: %s, ", (dvd_track.valid ? "yes" : " no"));
		printf("Filesize: % 5.0lf MBs", dvd_track.filesize_mbs);
		printf("\n");

		// Display video information
		if(d_video) {
			printf("        Video format: %s, ", dvd_video.format);
			printf("Aspect ratio: %s, ", dvd_video.aspect_ratio);
			printf("Width: %" PRIu16 ", ", dvd_video.width);
			printf("Height: %" PRIu16 ", ", dvd_video.height);
			printf("FPS: %s, ", dvd_video.fps);
			printf("Display format: %s", display_formats[dvd_video.df]);
			printf("\n");
		}

		// Display audio tracks
		if(d_audio && dvd_track.audio_tracks) {

			d_stream_num = 1;

			for(audio_track_ix = 0; audio_track_ix < dvd_track.audio_tracks; audio_track_ix++) {

				dvd_audio = dvd_track.dvd_audio_tracks[audio_track_ix];

				if(dvd_audio.active == false && !verbose)
					continue;

				printf("        Audio: %02" PRIu8 ", ", d_stream_num);
				printf("Language: %s, ", (strlen(dvd_audio.lang_code) ? dvd_audio.lang_code : "--"));
				printf("Codec: %s, ", dvd_audio.codec);
				printf("Channels: %" PRIu8 ", ", dvd_audio.channels);
				printf("Stream id: %s, ", dvd_audio.stream_id);
				printf("Active: %s", (dvd_audio.active ? "yes" : "no"));
				printf("\n");
				d_stream_num++;

			}

		}

		// Display chapters
		if(d_chapters && dvd_track.chapters) {

			for(chapter_ix = 0; chapter_ix < dvd_track.chapters; chapter_ix++) {

				dvd_chapter = dvd_track.dvd_chapters[chapter_ix];
				printf("        Chapter: %02" PRIu8 ", Length: %s\n", dvd_chapter.chapter, dvd_chapter.length);

			}

		}

		// Display track cells
		if(d_cells && dvd_track.cells) {

			for(cell_ix = 0; cell_ix < dvd_track.cells; cell_ix++) {

				dvd_cell = dvd_track.dvd_cells[cell_ix];

				printf("        Cell: %02" PRIu8 ", ", dvd_cell.cell);
				printf("Length: %s, ", dvd_cell.length);
				if(debug) {
					printf("First sector: %07" PRIu64 ", ", dvd_cell.first_sector);
					printf("Last sector: %07" PRIu64", ", dvd_cell.last_sector);
				}
				printf("Filesize: % 5.0lf MBs", dvd_cell.filesize_mbs);
				printf("\n");

			}

		}

		// Display subtitles
		if(d_subtitles && dvd_track.subtitles) {

			d_stream_num = 1;

			for(subtitle_track_ix = 0; subtitle_track_ix < dvd_track.subtitles; subtitle_track_ix++) {

				dvd_subtitle = dvd_track.dvd_subtitles[subtitle_track_ix];

				if(dvd_subtitle.active == false && !verbose)
					continue;

				printf("        Subtitle: %02" PRIu8 ", ", d_stream_num);
				printf("Language: %s, ", (strlen(dvd_subtitle.lang_code) ? dvd_subtitle.lang_code : "--"));
				printf("Stream id: %s, ", dvd_subtitle.stream_id);
				printf("Active: %s", (dvd_subtitle.active ? "yes" : "no"));
				printf("\n");

				d_stream_num++;

			}

		}

	}

	// Cleanup

	cleanup:

	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(vts_ifo)
		ifoClose(vts_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	return 0;

}
