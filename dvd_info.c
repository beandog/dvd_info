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
#include <dvdread/dvd_reader.h>
#include <dvdread/dvd_udf.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>
#include "dvd_info.h"
#include "dvd_device.h"
#include "dvd_vmg_ifo.h"
#include "dvd_track.h"
#ifdef __linux__
#include <linux/cdrom.h>
#include "dvd_drive.h"
#endif
#ifdef JSON_SUPPORT
#include <jansson.h>
#endif
#ifndef VERSION
#define VERSION "1.0"
#endif

void print_usage(char *binary) {

	printf("%s %s - GPL Copyright (c) 2014 Steve Dibb <steve.dibb@gmail.com>\n", binary, VERSION);
	printf("\n");
	printf("Usage: %s [options] [-t track number] [dvd path]\n", binary);
	printf("\n");
	printf("Options:\n");
#ifdef JSON_SUPPORT
	printf("  -j, --json		Display output in JSON format\n");
#endif
	printf("  -k, --ini		Display output in INI format\n");
	printf("  -t, --track [number]	Limit to one title track\n");
	printf("\n");
	printf("DVD path can be a directory, a device filename, or a local file.\n");
	printf("\n");
	printf("Examples:\n");
	printf("  dvd_info /dev/dvd	# Read a DVD drive directly\n");
	printf("  dvd_info movie.iso	# Read an image file\n");
	printf("  dvd_info movie/	# Read a directory that contains VIDEO_TS\n");
	printf("\n");
	printf("Default output is similar in syntax to 'lsdvd' program, and is\n");
#ifdef JSON_SUPPORT
	printf("not as verbose as INI or JSON formats.\n");
#else
	printf("not as verbose as INI format.\n");
#endif
	printf("\n");
	printf("If no DVD path is given, %s is used in its place.\n", DEFAULT_DVD_DEVICE);
	printf("\n");
	printf("See 'man dvd_info' for more details, or http://dvds.beandog.org/\n");

}

int main(int argc, char **argv) {

	// Program name
	bool p_dvd_info = false;
	bool p_dvd_xchap = false;
	char *program_name = basename(argv[0]);
	if(strncmp("dvd_xchap", program_name, 8) == 0)
		p_dvd_xchap = true;
	else
		p_dvd_info = true;

	// Display output
	int d_json = 0;
	int d_lsdvd = 1;
	int d_ini = 0;
	int d_debug = 0;

	// dvd_info
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
	useconds_t sleepy_time = 1000000;
	uint8_t num_naps = 0;
	uint8_t max_num_naps = 60;
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
	dvd_chapter.startcell = 1;

	// Cells
	struct dvd_cell dvd_cell;
	dvd_cell.cell = 1;
	memset(dvd_cell.length, '\0', sizeof(dvd_cell.length));
	snprintf(dvd_cell.length, DVD_CELL_LENGTH + 1, "00:00:00.000");
	dvd_cell.msecs = 0;

	// Statistics
	uint32_t longest_msecs = 0;

#ifdef JSON_SUPPORT
	// JSON variables
	json_t *json_dvd;
	json_t *json_dvd_info;
	json_t *json_dvd_tracks;
	json_t *json_dvd_track;
	json_t *json_dvd_video;
	json_t *json_dvd_audio_tracks;
	json_t *json_dvd_audio;
	json_t *json_dvd_subtitles;
	json_t *json_dvd_subtitle;
	json_t *json_dvd_chapters;
	json_t *json_dvd_chapter;
	json_t *json_dvd_cells;
	json_t *json_dvd_cell;

	json_dvd = json_object();
	json_dvd_info = json_object();
	json_dvd_tracks = json_array();
	json_dvd_track = json_object();
	json_dvd_video = json_object();
	json_dvd_audio_tracks = json_array();
	json_dvd_audio = json_object();
	json_dvd_subtitles = json_array();
	json_dvd_subtitle = json_object();
	json_dvd_chapters = json_array();
	json_dvd_chapter = json_object();
	json_dvd_cells = json_array();
	json_dvd_cell = json_object();
#endif

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
#ifdef JSON_SUPPORT
	str_options = "hjkt:z";
#else
	str_options = "hkt:z";
#endif

	struct option long_options[] = {

#ifdef JSON_SUPPORT
		{ "json", no_argument, & d_json, 1 },
#endif
		{ "ini", no_argument, & d_ini, 1 },
		{ "debug", no_argument, & d_debug, 1 },

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

#ifdef JSON_SUPPORT
			case 'j':
				d_json = 1;
				d_lsdvd = 0;
				d_ini = 0;
				d_debug = 0;
				break;
#endif

			case 'k':
				d_json = 0;
				d_lsdvd = 0;
				d_ini = 1;
				d_debug = 0;
				break;

			case 't':
				opt_track_number = true;
				arg_track_number = atoi(optarg);
				break;

			case 'z':
				d_json = 0;
				d_lsdvd = 0;
				d_ini = 0;
				d_debug = 1;
				break;

			// ignore unknown arguments
			case '?':
			// let getopt_long set the variable
			case 0:
			default:
				break;

		}

	}

	// Handle --json argument
	if(d_json || d_ini || d_debug)
		d_lsdvd = 0;

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

			fprintf(stderr, "waiting for media, will give up after one minute\n");
			while(!dvd_drive_has_media(device_filename) && (num_naps < max_num_naps)) {
				usleep(sleepy_time);
				num_naps = num_naps + 1;

				// This is slightly annoying, even for me.
				fprintf(stderr, "%i ", num_naps);

				// Tired of waiting, exiting out
				if(num_naps == max_num_naps) {
					fprintf(stderr, "\n");
					fprintf(stderr, "tired of waiting for media, quitting\n");
					return 1;
				}

			}

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

	// Open VMG IFO -- where all the cool stuff is
	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	if(!vmg_ifo || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "%s: Opening VMG IFO failed\n", program_name);
		DVDClose(dvdread_dvd);
		return 1;
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
			fprintf(stderr, "* Opening IFO %u\n", vts);

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(!vts_ifos[vts]) {
			fprintf(stderr, "%s: opening VTS IFO %u failed; skipping IFO\n", program_name, vts);
			valid_ifos[vts] = false;
			has_invalid_ifos = true;
			vts_ifos[vts] = NULL;
		} else if(!ifo_is_vts(vts_ifos[vts])) {
			fprintf(stderr, "%s: opening VTSI_MAT for VTS IFO %u failed; skipping IFO\n", program_name, vts);
			valid_ifos[vts] = false;
			has_invalid_ifos = true;
			ifoClose(vts_ifos[vts]);
			vts_ifos[vts] = NULL;
		} else {
			valid_ifos[vts] = true;
		}

	}

	// Exit if the track requested is on an invalid IFO
	if(has_invalid_ifos && opt_track_number) {

		vts = dvd_vts_ifo_number(vmg_ifo, track_number);

		if(valid_ifos[vts] == false)
			fprintf(stderr, "%s: the VTS IFO %u for title track %u is invalid, setting all values to zero.\n", program_name, vts, track_number);

	}

	// GRAB ALL THE THINGS
	dvd_info.side = dvd_info_side(vmg_ifo);
	strncpy(dvd_info.title, dvd_title(device_filename), DVD_TITLE);
	strncpy(dvd_info.provider_id, dvd_provider_id(vmg_ifo), DVD_PROVIDER_ID);
	strncpy(dvd_info.vmg_id, dvd_vmg_id(vmg_ifo), DVD_VMG_ID);
	strncpy(dvd_info.dvdread_id, dvd_dvdread_id(dvdread_dvd), DVD_DVDREAD_ID);

	/**
	 * Track information
	 */

	struct dvd_track dvd_tracks[dvd_info.tracks];

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		if(d_debug)
			printf("[Track %u]\n", track_number);

		// Open IFO
		dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, track_number);

		if(d_debug)
			printf("* IFO: %u\n", dvd_track.vts);

		// Set track values to empty if it is invalid
		if(valid_ifos[dvd_track.vts] == false) {
			fprintf(stderr, "%s: IFO %u for track %u is invalid, skipping track\n", program_name, dvd_track.vts, track_number);
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
		dvd_track.msecs = dvd_track_milliseconds(vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.chapters = dvd_track_chapters(vmg_ifo, vts_ifo, dvd_track.track);

		if(dvd_track.msecs > longest_msecs) {
			dvd_info.longest_track = dvd_track.track;
			longest_msecs = dvd_track.msecs;
		}

		/** Video **/

		strncpy(dvd_video.codec, dvd_track_video_codec(vts_ifo), DVD_VIDEO_CODEC);
		strncpy(dvd_video.format, dvd_track_video_format(vts_ifo), DVD_VIDEO_FORMAT);
		dvd_video.width = dvd_track_video_width(vts_ifo);
		dvd_video.height = dvd_track_video_height(vts_ifo);
		strncpy(dvd_video.aspect_ratio, dvd_track_video_aspect_ratio(vts_ifo), DVD_VIDEO_ASPECT_RATIO);
		dvd_video.letterbox = dvd_track_letterbox_video(vts_ifo);
		dvd_video.pan_and_scan = dvd_track_pan_scan_video(vts_ifo);
		dvd_video.df = dvd_track_permitted_df(vts_ifo);
		dvd_video.angles = dvd_track_angles(vmg_ifo, dvd_track.track);
		dvd_track.audio_tracks = dvd_track_num_audio_streams(vts_ifo);
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
				dvd_audio.active = dvd_track_active_audio_stream(vmg_ifo, vts_ifo, dvd_track.track, dvd_audio.track);
				if(dvd_audio.active)
					dvd_track.active_audio++;
				strncpy(dvd_audio.lang_code, dvd_track_audio_lang_code(vts_ifo, c), DVD_AUDIO_LANG_CODE);
				strncpy(dvd_audio.codec, dvd_track_audio_codec(vts_ifo, c), DVD_AUDIO_CODEC);
				dvd_audio.channels = dvd_track_audio_num_channels(vts_ifo, c);
				strncpy(dvd_audio.stream_id, dvd_track_audio_stream_id(vts_ifo, c), DVD_AUDIO_STREAM_ID);

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
				dvd_subtitle.active = dvd_track_active_subtitle(vmg_ifo, vts_ifo, dvd_track.track, dvd_subtitle.track);
				if(dvd_subtitle.active)
					dvd_track.active_subs++;
				strncpy(dvd_subtitle.stream_id, dvd_track_subtitle_stream_id(c), DVD_SUBTITLE_STREAM_ID);
				strncpy(dvd_subtitle.lang_code, dvd_track_subtitle_lang_code(vts_ifo, c), DVD_SUBTITLE_LANG_CODE);

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
				dvd_chapter.msecs = dvd_chapter_milliseconds(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
				dvd_chapter.startcell = dvd_chapter_startcell(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);

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
				dvd_cell.msecs = dvd_cell_milliseconds(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);

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
	if(p_dvd_info && d_lsdvd == 1) {

		printf("Disc Title: %s\n", dvd_info.title);

		for(track_number = d_first_track; track_number <= d_last_track; track_number++) {
			dvd_track = dvd_tracks[track_number - 1];
			printf("Title: %02u, ", dvd_track.track);
			printf("Length: %s ", dvd_track.length);
			printf("Chapters: %02u, ", dvd_track.chapters);
			printf("Cells: %02u, ", dvd_track.cells);
			printf("Audio streams: %02u, ", dvd_track.active_audio);
			printf("Subpictures: %02u\n", dvd_track.active_subs);
		}

		if(d_all_tracks)
			printf("Longest track: %02u\n", dvd_info.longest_track);

	}

#ifdef JSON_SUPPORT
	/** JSON display output **/

	if(p_dvd_info && d_json == 1) {

		// DVD
		json_object_set_new(json_dvd_info, "title", json_string(dvd_info.title));
		json_object_set_new(json_dvd_info, "side", json_integer(dvd_info.side));
		json_object_set_new(json_dvd_info, "tracks", json_integer(dvd_info.tracks));
		json_object_set_new(json_dvd_info, "longest track", json_integer(dvd_info.longest_track));
		if(strlen(dvd_info.provider_id))
			json_object_set_new(json_dvd_info, "provider id", json_string(dvd_info.provider_id));
		if(strlen(dvd_info.vmg_id))
			json_object_set_new(json_dvd_info, "vmg id", json_string(dvd_info.vmg_id));
		json_object_set_new(json_dvd_info, "video title sets", json_integer(dvd_info.video_title_sets));
		json_object_set_new(json_dvd_info, "dvdread id", json_string(dvd_info.dvdread_id));

		for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

			dvd_track = dvd_tracks[track_number - 1];

			json_dvd_track = json_object();
			json_object_set_new(json_dvd_track, "track", json_integer(dvd_track.track));

			// If the title track is invalid, skip to the next one
			json_object_set_new(json_dvd_track, "valid", json_integer(dvd_track.valid));
			if(dvd_track.valid == 0) {
				json_array_append(json_dvd_tracks, json_dvd_track);
				continue;
			}

			dvd_video = dvd_tracks[track_number - 1].dvd_video;

			json_dvd_video = json_object();

			json_object_set_new(json_dvd_track, "length", json_string(dvd_track.length));
			json_object_set_new(json_dvd_track, "msecs", json_integer(dvd_track.msecs));
			json_object_set_new(json_dvd_track, "vts", json_integer(dvd_track.vts));
			json_object_set_new(json_dvd_track, "ttn", json_integer(dvd_track.ttn));

			if(strlen(dvd_video.codec))
				json_object_set_new(json_dvd_video, "codec", json_string(dvd_video.codec));
			if(strlen(dvd_video.format))
				json_object_set_new(json_dvd_video, "format", json_string(dvd_video.format));
			if(strlen(dvd_video.aspect_ratio))
				json_object_set_new(json_dvd_video, "aspect ratio", json_string(dvd_video.aspect_ratio));
			json_object_set_new(json_dvd_video, "width", json_integer(dvd_video.width));
			json_object_set_new(json_dvd_video, "height", json_integer(dvd_video.height));
			// FIXME needs cleanup
			/*
			if(dvd_video.df == 0)
				json_object_set_new(json_dvd_video, "df", json_string("Pan and Scan + Letterbox"));
			else if(dvd_video.df == 1)
				json_object_set_new(json_dvd_video, "df", json_string("Pan and Scan"));
			else if(dvd_video.df == 2)
				json_object_set_new(json_dvd_video, "df", json_string("Letterbox"));
			*/
			json_object_set_new(json_dvd_video, "angles", json_integer(dvd_video.angles));
			// Only display FPS if it's been populated as a string
			if(strlen(dvd_video.fps))
				json_object_set_new(json_dvd_video, "fps", json_string(dvd_video.fps));
			// FIXME display permitted df instead, since displaying
			// "letterbox" or "pan and scan" is subjective, and
			// possibly incorrect
			// json_object_set_new(json_dvd_video, "letterbox", json_integer(dvd_video.letterbox));
			// json_object_set_new(json_dvd_video, "pan and scan", json_integer(dvd_video.pan_and_scan));
			json_object_set_new(json_dvd_track, "video", json_dvd_video);

			// Audio tracks

			if(dvd_track.audio_tracks) {

				json_dvd_audio_tracks = json_array();

				for(c = 0; c < dvd_track.audio_tracks; c++) {

					dvd_audio = dvd_track.dvd_audio_tracks[c];

					json_dvd_audio = json_object();

					json_object_set_new(json_dvd_audio, "track", json_integer(dvd_audio.track));
					json_object_set_new(json_dvd_audio, "active", json_integer(dvd_audio.active));
					if(strlen(dvd_audio.lang_code) == DVD_AUDIO_LANG_CODE)
						json_object_set_new(json_dvd_audio, "lang code", json_string(dvd_audio.lang_code));
					json_object_set_new(json_dvd_audio, "codec", json_string(dvd_audio.codec));
					json_object_set_new(json_dvd_audio, "channels", json_integer(dvd_audio.channels));
					json_object_set_new(json_dvd_audio, "stream id", json_string(dvd_audio.stream_id));
					json_array_append(json_dvd_audio_tracks, json_dvd_audio);

				}

				json_object_set_new(json_dvd_track, "audio", json_dvd_audio_tracks);

			}

			// Subtitles

			if(dvd_track.subtitles) {

				json_dvd_subtitles = json_array();

				for(c = 0; c < dvd_track.subtitles; c++) {

					dvd_subtitle = dvd_track.dvd_subtitles[c];

					json_dvd_subtitle = json_object();

					json_object_set_new(json_dvd_subtitle, "track", json_integer(dvd_subtitle.track));
					json_object_set_new(json_dvd_subtitle, "active", json_integer(dvd_subtitle.active));
					if(strlen(dvd_subtitle.lang_code) == DVD_SUBTITLE_LANG_CODE)
						json_object_set_new(json_dvd_subtitle, "lang code", json_string(dvd_subtitle.lang_code));
					json_object_set_new(json_dvd_subtitle, "stream id", json_string(dvd_subtitle.stream_id));
					json_array_append(json_dvd_subtitles, json_dvd_subtitle);

				}

				json_object_set_new(json_dvd_track, "subtitles", json_dvd_subtitles);

			}

			// Chapters

			if(dvd_track.chapters) {

				json_dvd_chapters = json_array();

				for(c = 0; c < dvd_track.chapters; c++) {

					dvd_chapter = dvd_track.dvd_chapters[c];

					json_dvd_chapter = json_object();

					json_object_set_new(json_dvd_chapter, "chapter", json_integer(dvd_chapter.chapter));
					json_object_set_new(json_dvd_chapter, "length", json_string(dvd_chapter.length));
					json_object_set_new(json_dvd_chapter, "msecs", json_integer(dvd_chapter.msecs));
					json_object_set_new(json_dvd_chapter, "startcell", json_integer(dvd_chapter.startcell));
					json_array_append(json_dvd_chapters, json_dvd_chapter);

				}

				json_object_set_new(json_dvd_track, "chapters", json_dvd_chapters);

			}

			// Cells

			if(dvd_track.cells) {

				json_dvd_cells = json_array();

				for(c = 0; c < dvd_track.cells; c++) {

					dvd_cell = dvd_track.dvd_cells[c];

					json_dvd_cell = json_object();

					json_object_set_new(json_dvd_cell, "cell", json_integer(dvd_cell.cell));
					json_object_set_new(json_dvd_cell, "length", json_string(dvd_cell.length));
					json_object_set_new(json_dvd_cell, "msecs", json_integer(dvd_cell.msecs));
					json_array_append(json_dvd_cells, json_dvd_cell);

				}

				json_object_set_new(json_dvd_track, "cells", json_dvd_cells);

			}

			json_array_append(json_dvd_tracks, json_dvd_track);

		}

		json_object_set_new(json_dvd, "dvd", json_dvd_info);

		if(track_number)
			json_object_set_new(json_dvd, "tracks", json_dvd_tracks);

		printf("%s\n", json_dumps(json_dvd, JSON_INDENT(1) + JSON_PRESERVE_ORDER));

	}
#endif

	// INI style format

	if(p_dvd_info && d_ini == 1) {

		printf("[dvd]\n");
		printf("title = %s\n", dvd_info.title);
		printf("side = %u\n", dvd_info.side);
		printf("tracks = %u\n", dvd_info.tracks);
		printf("longest track = %u\n", dvd_info.longest_track);
		if(strlen(dvd_info.provider_id))
			printf("provider id = %s\n", dvd_info.provider_id);
		if(strlen(dvd_info.vmg_id))
			printf("vmg id = %s\n", dvd_info.vmg_id);
		printf("video title sets = %u\n", dvd_info.video_title_sets);
		printf("dvdread id = %s\n", dvd_info.dvdread_id);

		// Title tracks

		for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

			dvd_track = dvd_tracks[track_number - 1];

			printf("\n");
			printf("[track %u]\n", dvd_track.track);
			printf("track = %u\n", dvd_track.track);
			printf("valid = %s\n", dvd_track.valid ? "yes" : "no");

			// If the title track is invalid, skip to the next one
			if(dvd_track.valid == 0) {
				continue;
			}


			printf("length = %s\n", dvd_track.length);
			printf("msecs = %u\n", dvd_track.msecs);
			printf("vts = %u\n", dvd_track.vts);
			printf("ttn = %u\n", dvd_track.ttn);

			dvd_video = dvd_track.dvd_video;

			if(strlen(dvd_video.codec))
				printf("codec = %s\n", dvd_video.codec);
			if(strlen(dvd_video.format))
				printf("format = %s\n", dvd_video.format);
			if(strlen(dvd_video.aspect_ratio))
				printf("aspect ratio = %s\n", dvd_video.aspect_ratio);
			printf("width = %u\n", dvd_video.width);
			printf("height = %u\n", dvd_video.height);
			printf("angles = %u\n", dvd_video.angles);
			if(strlen(dvd_video.fps))
				printf("fps = %s\n", dvd_video.fps);
			printf("audio tracks = %u\n", dvd_track.audio_tracks);
			printf("subtitles = %u\n", dvd_track.subtitles);
			printf("chapters = %u\n", dvd_track.chapters);

			// Audio tracks

			for(c = 0; c < dvd_track.audio_tracks; c++) {

				dvd_audio = dvd_track.dvd_audio_tracks[c];

				printf("\n");
				printf("[track %u audio track %u]\n", dvd_track.track, dvd_audio.track);
				printf("active = %s\n", dvd_audio.active == 0 ? "no" : "yes");
				printf("channels = %u\n", dvd_audio.channels);
				printf("codec = %s\n", dvd_audio.codec);
				if(strlen(dvd_audio.lang_code))
					printf("lang code = %s\n", dvd_audio.lang_code);
				printf("stream id = %s\n", dvd_audio.stream_id);

			}

			// Subtitles

			for(c = 0; c < dvd_track.subtitles; c++) {

				dvd_subtitle = dvd_track.dvd_subtitles[c];

				printf("\n");
				printf("[track %u subtitle track %u]\n", dvd_track.track, dvd_subtitle.track);
				printf("track = %u\n", dvd_subtitle.track);
				printf("active = %s\n", dvd_subtitle.active == 0 ? "no" : "yes");
				if(strlen(dvd_subtitle.lang_code))
					printf("lang code = %s\n", dvd_subtitle.lang_code);
				printf("stream id = %s\n", dvd_subtitle.stream_id);

			}

			// Chapters

			if(dvd_track.chapters) {

				for(c = 0; c < dvd_track.chapters; c++) {

					dvd_chapter = dvd_track.dvd_chapters[c];

					printf("\n");
					printf("[track %u chapter %u]\n", dvd_track.track, dvd_chapter.chapter);
					printf("chapter = %u\n", dvd_chapter.chapter);
					printf("length = %s\n", dvd_chapter.length);
					printf("msecs = %u\n", dvd_chapter.msecs);
					printf("startcell = %u\n", dvd_chapter.startcell);

				}

			}

			// Cells

			if(dvd_track.cells) {

				for(c = 0; c < dvd_track.cells; c++) {

					dvd_cell = dvd_track.dvd_cells[c];

					printf("\n");
					printf("[track %u cell %u]\n", dvd_track.track, dvd_cell.cell);
					printf("cell = %u\n", dvd_cell.cell);
					printf("length = %s\n", dvd_cell.length);
					printf("msecs = %u\n", dvd_cell.msecs);

				}

			}

		}

	}

	// Cleanup

	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(vts_ifo)
		ifoClose(vts_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	return 0;

}
