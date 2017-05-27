#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <inttypes.h>
#include <libgen.h>
#include "dvd_info.h"
#include "dvd_device.h"
#include "dvd_vmg_ifo.h"
#include "dvd_track.h"
#include "dvd_chapter.h"
#include "dvd_cell.h"
#include "dvd_video.h"
#include "dvd_audio.h"
#include "dvd_subtitles.h"
#include "dvd_time.h"
#include "dvd_json.h"
#include "dvd_xchap.h"
#include "dvd_debug.h"
#ifdef __linux__
#include <linux/cdrom.h>
#include "dvd_drive.h"
#endif
#ifndef VERSION
#define VERSION "1.0"
#endif

void print_usage(char *binary) {

	printf("%s %s - display information about a DVD\n", binary, VERSION);
	printf("\n");
	printf("Usage: %s [options] [-t track number] [dvd path]\n", binary);
	printf("\n");
	printf("Options:\n");
	printf("  -t, --track [number]	Limit to one title track\n");
	printf("\n");
	printf("Extra information:\n");
	printf("  -a, --audio		audio streams\n");
	printf("  -v, --video		video\n");
	printf("  -c, --chapters	chapters\n");
	printf("  -s, --subtitles	subtitles\n");
	printf("  -d, --cells		cells\n");
	printf("\n");
	printf("Display tracks with features (default output only):\n");
	printf("  --ntsc		Video format is NTSC\n");
	printf("  --pal			Video format is PAL\n");
	printf("\n");
	printf("DVD path can be a directory, a device filename, or a local file.\n");
	printf("\n");
	printf("Examples:\n");
	printf("  dvd_info /dev/dvd	# Read a DVD drive directly\n");
	printf("  dvd_info movie.iso	# Read an image file\n");
	printf("  dvd_info movie/	# Read a directory that contains VIDEO_TS\n");
	printf("\n");
	printf("Default output is similar in syntax to 'lsdvd' program, and is\n");
	printf("not as verbose as JSON's format.\n");
	printf("\n");
	printf("If no DVD path is given, %s is used in its place.\n", DEFAULT_DVD_DEVICE);
	printf("\n");
	printf("See 'man dvd_info' for more details, or http://dvds.beandog.org/\n");

}

int main(int argc, char **argv) {

	// Program name
	bool p_dvd_info = false;
	bool p_dvd_xchap = false;
	bool p_dvd_debug = false;
	bool p_dvd_id = false;
	bool p_dvd_title = false;
	bool p_dvd_json = false;
	char *program_name = basename(argv[0]);
	if(strncmp("dvd_xchap", program_name, 8) == 0)
		p_dvd_xchap = true;
	else if(strncmp("dvd_id", program_name, 6) == 0)
		p_dvd_id = true;
	else if(strncmp("dvd_debug", program_name, 9) == 0)
		p_dvd_debug = true;
	else if(strncmp("dvd_title", program_name, 9) == 0)
		p_dvd_title = true;
	else if(strncmp("dvd_json", program_name, 8) == 0)
		p_dvd_json = true;
	else
		p_dvd_info = true;
	int retval = 0;

	// Display output
	int d_debug = 0;

	// lsdvd display output
	int d_audio = 0;
	int d_video = 0;
	int d_chapters = 0;
	int d_subtitles = 0;
	int d_cells = 0;
	int d_all = 0;

	// dvd_query
	int d_ntsc = 0;
	int d_pal = 0;

	// dvd_info
	char dvdread_id[DVD_DVDREAD_ID + 1] = {'\0'};
	bool d_all_tracks = true;
	uint16_t d_first_track = 1;
	uint16_t d_last_track = 1;
	uint16_t track_number = 1;
	uint16_t vts = 1;
	bool has_invalid_ifos = false;
	uint8_t c = 0;

	// Device hardware
	int dvd_fd = 0;
	const char *device_filename = NULL;
	bool is_hardware = false;

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

	// Video Title Set
	struct dvd_vts dvd_vts;
	dvd_vts.vts = 1;
	memset(dvd_vts.id, '\0', sizeof(dvd_vts.id));

	// Track
	struct dvd_track dvd_track;
	dvd_track.track = 1;
	dvd_track.valid = 1;
	dvd_track.vts = 1;
	dvd_track.ttn = 1;
	snprintf(dvd_track.length, DVD_TRACK_LENGTH + 1, "00:00:00.000");
	dvd_track.msecs = 0;
	dvd_track.chapters = 0;
	dvd_track.audio_tracks = 0;
	dvd_track.active_audio = 0;
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
	dvd_video.df = 0;
	memset(dvd_video.fps, '\0', sizeof(dvd_video.fps));
	dvd_video.angles = 1;

	// Audio
	struct dvd_audio dvd_audio;
	dvd_audio.track = 1;
	dvd_audio.active = 0;
	memset(dvd_audio.stream_id, '\0', sizeof(dvd_audio.stream_id));
	memset(dvd_audio.lang_code, '\0', sizeof(dvd_audio.lang_code));
	memset(dvd_audio.codec, '\0', sizeof(dvd_audio.codec));
	dvd_audio.channels = 0;

	// Subtitles
	struct dvd_subtitle dvd_subtitle;
	dvd_subtitle.track = 1;
	dvd_subtitle.active = 0;
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

	// Display formats
	const char *display_formats[4] = { "Pan and Scan or Letterbox", "Pan and Scan", "Letterbox", "Unset" };

	// Statistics
	uint32_t longest_msecs = 0;

	// getopt_long
	bool opt_track_number = false;
	int arg_track_number = 0;
	int long_index = 0;
	int opt = 0;
	// Send 'invalid argument' to stderr
	opterr= 1;
	// Check for invalid input
	bool valid_args = true;
	// Not enabled by an argument, set internally
	// I could probably come up with a better variable name. I probably would if
	// I understood getopt better. :T
	const char *str_options;
	str_options = "acdhst:vxz";

	struct option long_options[] = {

		{ "debug", no_argument, & d_debug, 1 },
		{ "audio", no_argument, & d_audio, 1 },
		{ "video", no_argument, & d_video, 1 },
		{ "chapters", no_argument, & d_chapters, 1 },
		{ "subtitles", no_argument, & d_subtitles, 1 },
		{ "cells", no_argument, & d_cells, 1 },
		{ "all", no_argument, & d_all, 1 },

		// dvd_query
		{ "ntsc", no_argument, & d_ntsc, 1 },
		{ "pal", no_argument, & d_pal, 1 },

		// Entries with both a name and a value, will take either the
		// long option or the short one.  Fex, '--device' or '-i'
		{ "track", required_argument, 0, 't' },
		{ 0, 0, 0, 0 }

	};

	// parse options
	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {

		// It's worth noting that if there are unknown options passed,
		// I just ignore them, and continue printing requested data.
		switch(opt) {

			case 'h':
				print_usage(program_name);
				return 0;

			case 'a':
				d_audio = 1;
				break;

			case 'c':
				d_chapters = 1;
				break;

			case 'd':
				d_cells = 1;
				break;

			case 's':
				d_subtitles = 1;
				break;

			case 't':
				opt_track_number = true;
				arg_track_number = atoi(optarg);
				break;

			case 'v':
				d_video = 1;
				break;

			case 'x':
				d_audio = 1;
				d_video = 1;
				d_chapters = 1;
				d_subtitles = 1;
				d_cells = 1;
				break;

			case 'z':
				d_debug = 1;
				break;

			// ignore unknown arguments
			case '?':
				print_usage(program_name);
				return 1;
			// let getopt_long set the variable
			case 0:
			default:
				break;

		}

	}

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
		fprintf(stderr, "%s: cannot access %s\n", program_name, device_filename);
		return 1;
	}

	// Check to see if device can be opened
	dvd_fd = dvd_device_open(device_filename);
	if(dvd_fd < 0) {
		fprintf(stderr, "%s: error opening %s\n", program_name, device_filename);
		return 1;
	}
	dvd_device_close(dvd_fd);

	// Check if it is hardware or an image file
	is_hardware = dvd_device_is_hardware(device_filename);

#ifdef __linux__

	// Poll drive status if it is hardware
	if(is_hardware) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "drive status: ");
			dvd_drive_display_status(device_filename);

			return 1;

		}

	}

#endif

	// begin libdvdread usage

	// Open DVD device
	dvdread_dvd = DVDOpen(device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "%s: Opening DVD %s failed\n", program_name, device_filename);
		return 1;
	}

	// Check if DVD has an identifier, fail otherwise
	strncpy(dvdread_id, dvd_dvdread_id(dvdread_dvd), DVD_DVDREAD_ID);
	if(strlen(dvdread_id) == 0) {
		fprintf(stderr, "%s: Opening DVD %s failed\n", program_name, device_filename);
		return 1;
	}

	// dvd_id program
	if(p_dvd_id) {
		printf("%s\n", dvdread_id);
		DVDClose(dvdread_dvd);
		return 0;
	}

	// dvd_title program
	if(p_dvd_title) {
		printf("%s\n", dvd_title(device_filename));
		DVDClose(dvdread_dvd);
		return 0;
	}

	// Open VMG IFO -- where all the cool stuff is
	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	if(!vmg_ifo || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "%s: Opening VMG IFO failed\n", program_name);
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Run dvd_debug
	if(p_dvd_debug) {
		retval = dvd_debug(dvdread_dvd);
		DVDClose(dvdread_dvd);
		return retval;
	}

	// Get the total number of title tracks on the DVD
	dvd_info.tracks = dvd_tracks(vmg_ifo);

	// Exit if track number requested does not exist
	if(opt_track_number && (arg_track_number > dvd_info.tracks || arg_track_number < 1)) {
		fprintf(stderr, "%s: Invalid track number %d\n", program_name, arg_track_number);
		fprintf(stderr, "%s: Valid track numbers: 1 to %u\n", program_name, dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	} else if(opt_track_number) {
		d_first_track = (uint16_t)arg_track_number;
		d_last_track = (uint16_t)arg_track_number;
		track_number = d_first_track;
		d_all_tracks = false;
	} else {
		d_first_track = 1;
		d_last_track = dvd_info.tracks;
		d_all_tracks = true;
	}

	// Exit if all the IFOs cannot be opened
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	bool valid_ifos[dvd_info.video_title_sets];
	ifo_handle_t *vts_ifos[dvd_info.video_title_sets + 1];
	vts_ifos[0] = NULL;

	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		if(d_debug)
			fprintf(stderr, "[DEBUG] %s: Opening IFO %u\n", program_name, vts);

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(vts_ifos[vts] == NULL) {
			if(d_debug)
				fprintf(stderr, "[DEBUG] %s: opening VTS IFO %u failed; skipping IFO\n", program_name, vts);
			valid_ifos[vts] = false;
			has_invalid_ifos = true;
			vts_ifos[vts] = NULL;
		} else if(!ifo_is_vts(vts_ifos[vts])) {
			if(d_debug)
				fprintf(stderr, "[DEBUG] %s: opening VTSI_MAT for VTS IFO %u failed; skipping IFO\n", program_name, vts);
			valid_ifos[vts] = false;
			has_invalid_ifos = true;
			ifoClose(vts_ifos[vts]);
			vts_ifos[vts] = NULL;
		} else {
			valid_ifos[vts] = true;
		}

	}

	if(has_invalid_ifos)
		fprintf(stderr, "[NOTICE] %s: You can safely ignore \"Invalid IFOs\" warnings since we work around them :)\n", program_name);

	// Exit if the track requested is on an invalid IFO
	if(has_invalid_ifos && opt_track_number) {

		vts = dvd_vts_ifo_number(vmg_ifo, track_number);

		if(valid_ifos[vts] == false && d_debug)
			fprintf(stderr, "[DEBUG] %s: the VTS IFO %u for title track %u is invalid, setting all values to zero.\n", program_name, vts, track_number);

	}

	// GRAB ALL THE THINGS
	dvd_info.side = dvd_info_side(vmg_ifo);
	strncpy(dvd_info.title, dvd_title(device_filename), DVD_TITLE);
	strncpy(dvd_info.provider_id, dvd_provider_id(vmg_ifo), DVD_PROVIDER_ID);
	strncpy(dvd_info.vmg_id, dvd_vmg_id(vmg_ifo), DVD_VMG_ID);
	strncpy(dvd_info.dvdread_id, dvdread_id, DVD_DVDREAD_ID);

	/**
	 * Track information
	 */

	struct dvd_track dvd_tracks[dvd_info.tracks];

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		// Open IFO
		dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, track_number);

		// Set track values to empty if it is invalid
		if(valid_ifos[dvd_track.vts] == false) {

			if(d_debug)
				fprintf(stderr, "[DEBUG] %s: IFO %u for track %u is invalid, skipping track\n", program_name, dvd_track.vts, track_number);

			dvd_track.track = track_number;
			dvd_track.valid = 0;

			dvd_track.ttn = dvd_track_ttn(vmg_ifo, dvd_track.track);
			snprintf(dvd_track.length, DVD_TRACK_LENGTH + 1, "00:00:00.000");
			dvd_track.msecs = 0;
			dvd_track.chapters = 0;
			dvd_track.audio_tracks = 0;
			dvd_track.active_audio = 0;
			dvd_track.subtitles = 0;
			dvd_track.active_subs = 0;
			dvd_track.cells = 0;

			dvd_tracks[track_number - 1] = dvd_track;

			continue;

		}

		vts_ifo = vts_ifos[dvd_track.vts];

		dvd_track.track = track_number;
		dvd_track.valid = 1;
		dvd_track.ttn = dvd_track_ttn(vmg_ifo, dvd_track.track);
		memset(dvd_track.length, '\0', sizeof(dvd_track.length));
		strncpy(dvd_track.length, dvd_track_length(vmg_ifo, vts_ifo, dvd_track.track), DVD_TRACK_LENGTH);
		dvd_track.msecs = dvd_track_msecs(vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.chapters = dvd_track_chapters(vmg_ifo, vts_ifo, dvd_track.track);

		if(dvd_track.msecs > longest_msecs) {
			dvd_info.longest_track = dvd_track.track;
			longest_msecs = dvd_track.msecs;
		}

		/** Video **/

		strncpy(dvd_video.codec, dvd_video_codec(vts_ifo), DVD_VIDEO_CODEC);
		strncpy(dvd_video.format, dvd_track_video_format(vts_ifo), DVD_VIDEO_FORMAT);
		dvd_video.width = dvd_video_width(vts_ifo);
		dvd_video.height = dvd_video_height(vts_ifo);
		strncpy(dvd_video.aspect_ratio, dvd_video_aspect_ratio(vts_ifo), DVD_VIDEO_ASPECT_RATIO);
		dvd_video.letterbox = dvd_video_letterbox(vts_ifo);
		dvd_video.pan_and_scan = dvd_video_pan_scan(vts_ifo);
		dvd_video.df = dvd_video_df(vts_ifo);
		dvd_video.angles = dvd_video_angles(vmg_ifo, dvd_track.track);
		dvd_track.audio_tracks = dvd_track_audio_tracks(vts_ifo);
		dvd_track.active_audio = 0;
		dvd_track.subtitles = dvd_track_subtitles(vts_ifo);
		dvd_track.active_subs = 0;
		dvd_track.cells = dvd_track_cells(vmg_ifo, vts_ifo, dvd_track.track);
		strncpy(dvd_video.fps, dvd_track_str_fps(vmg_ifo, vts_ifo, dvd_track.track), DVD_VIDEO_FPS);

		dvd_track.dvd_video = dvd_video;

		/** Audio Streams **/

		dvd_track.dvd_audio_tracks = calloc(dvd_track.audio_tracks, sizeof(*dvd_track.dvd_audio_tracks));

		if(dvd_track.audio_tracks && dvd_track.dvd_audio_tracks != NULL) {

			for(c = 0; c < dvd_track.audio_tracks; c++) {

				memset(&dvd_audio, 0, sizeof(dvd_audio));
				memset(dvd_audio.lang_code, '\0', sizeof(dvd_audio.lang_code));
				memset(dvd_audio.codec, '\0', sizeof(dvd_audio.codec));

				dvd_audio.track = c + 1;
				dvd_audio.active = dvd_audio_active(vmg_ifo, vts_ifo, dvd_track.track, dvd_audio.track);
				if(dvd_audio.active)
					dvd_track.active_audio++;
				strncpy(dvd_audio.lang_code, dvd_audio_lang_code(vts_ifo, c), DVD_AUDIO_LANG_CODE);
				strncpy(dvd_audio.codec, dvd_audio_codec(vts_ifo, c), DVD_AUDIO_CODEC);
				dvd_audio.channels = dvd_audio_channels(vts_ifo, c);
				strncpy(dvd_audio.stream_id, dvd_audio_stream_id(vts_ifo, c), DVD_AUDIO_STREAM_ID);

				dvd_track.dvd_audio_tracks[c] = dvd_audio;

			}

		}

		/** Subtitles **/

		dvd_track.dvd_subtitles = calloc(dvd_track.subtitles, sizeof(*dvd_track.dvd_subtitles));

		if(dvd_track.subtitles && dvd_track.dvd_subtitles != NULL) {

			for(c = 0; c < dvd_track.subtitles; c++) {

				memset(&dvd_subtitle, 0, sizeof(dvd_subtitle));
				memset(dvd_subtitle.lang_code, '\0', sizeof(dvd_subtitle.lang_code));

				dvd_subtitle.track = c + 1;
				dvd_subtitle.active = dvd_subtitle_active(vmg_ifo, vts_ifo, dvd_track.track, dvd_subtitle.track);
				if(dvd_subtitle.active)
					dvd_track.active_subs++;
				strncpy(dvd_subtitle.stream_id, dvd_subtitle_stream_id(c), DVD_SUBTITLE_STREAM_ID);
				strncpy(dvd_subtitle.lang_code, dvd_subtitle_lang_code(vts_ifo, c), DVD_SUBTITLE_LANG_CODE);

				dvd_track.dvd_subtitles[c] = dvd_subtitle;

			}

		}

		/** Chapters **/

		dvd_track.dvd_chapters = calloc(dvd_track.chapters, sizeof(*dvd_track.dvd_chapters));

		if(dvd_track.chapters && dvd_track.dvd_chapters != NULL) {

			for(c = 0; c < dvd_track.chapters; c++) {

				dvd_chapter.chapter = c + 1;

				memset(dvd_chapter.length, '\0', sizeof(dvd_chapter.length));
				strncpy(dvd_chapter.length, dvd_chapter_length(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter), DVD_CHAPTER_LENGTH);
				dvd_chapter.msecs = dvd_chapter_msecs(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
				dvd_chapter.first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
				dvd_chapter.last_cell = dvd_chapter_last_cell(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);

				dvd_track.dvd_chapters[c] = dvd_chapter;

			};

		}

		/** Cells **/

		dvd_track.dvd_cells = calloc(dvd_track.cells, sizeof(*dvd_track.dvd_cells));

		if(dvd_track.cells && dvd_track.dvd_cells != NULL) {

			for(c = 0; c < dvd_track.cells; c++) {

				dvd_cell.cell = c + 1;

				memset(dvd_cell.length, '\0', sizeof(dvd_cell.length));
				strncpy(dvd_cell.length, dvd_cell_length(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell), DVD_CELL_LENGTH);
				dvd_cell.msecs = dvd_cell_msecs(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
				dvd_cell.first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
				dvd_cell.last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);

				dvd_track.dvd_cells[c] = dvd_cell;

			}

		}

		dvd_tracks[track_number - 1] = dvd_track;

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
	if(p_dvd_info) {

		printf("Disc Title: %s\n", dvd_info.title);

		for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

			// dvd_query - limit to video format
			if(d_ntsc && strncmp(dvd_video.format, "NTSC", 4) != 0)
				continue;
			if(d_pal && strncmp(dvd_video.format, "PAL", 3) != 0)
				continue;

			// Display track information
			dvd_track = dvd_tracks[track_number - 1];
			printf("Track: %02u ", dvd_track.track);
			printf("Length: %s ", dvd_track.length);
			printf("Chapters: %02u ", dvd_track.chapters);
			printf("Cells: %02u ", dvd_track.cells);
			printf("Audio streams: %02u ", dvd_track.active_audio);
			printf("Subpictures: %02u\n", dvd_track.active_subs);

			// Display video information
			if(d_video) {
				printf("	Video format: %s Aspect ratio: %s Width: %u Height: %u Display format: %s FPS: %s Angles: %u\n", dvd_video.format, dvd_video.aspect_ratio, dvd_video.width, dvd_video.height, display_formats[dvd_video.df], dvd_video.fps, dvd_video.angles);
			}

			// Display audio tracks
			if(d_audio && dvd_track.audio_tracks) {

				for(c = 0; c < dvd_track.audio_tracks; c++) {

					dvd_audio = dvd_track.dvd_audio_tracks[c];
					printf("        Audio: %02u Language: %s Codec: %s Channels: %u Stream id: %s Active: %s\n", dvd_audio.track, (strlen(dvd_audio.lang_code) ? dvd_audio.lang_code : "--"), dvd_audio.codec, dvd_audio.channels, dvd_audio.stream_id, (dvd_audio.active ? "yes" : "no"));

				}

			}


			// Display chapters
			if(d_chapters && dvd_track.chapters) {

				for(c = 0; c < dvd_track.chapters; c++) {

					dvd_chapter = dvd_track.dvd_chapters[c];
					printf("        Chapter: %02u Length: %s\n", dvd_chapter.chapter, dvd_chapter.length);


				}

			}

			// Display track cells
			if(d_cells && dvd_track.cells) {

				for(c = 0; c < dvd_track.cells; c++) {

					dvd_cell = dvd_track.dvd_cells[c];
					printf("	Cell: %02u Length: %s\n", dvd_cell.cell, dvd_cell.length);

				}

			}

		}


		if(d_all_tracks)
			printf("Longest track: %02u\n", dvd_info.longest_track);

	}

	/** JSON display output **/

	if(p_dvd_json)
		dvd_json(dvd_info, dvd_tracks, track_number, d_first_track, d_last_track);

	/** Display DVD chapters **/
	track_number = (opt_track_number ? d_first_track : dvd_info.longest_track);
	if(p_dvd_xchap)
		dvd_xchap(dvd_tracks[track_number - 1]);

	// Cleanup

	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(vts_ifo)
		ifoClose(vts_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	return 0;

}
