#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include "config.h"
#include "dvd_info.h"
#include "dvd_open.h"
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
#include "dvd_init.h"
#ifdef __linux__
#include <linux/cdrom.h>
#include <linux/limits.h>
#include "dvd_drive.h"
#else
#include <limits.h>
#endif

	/**
	 *      _          _     _        __
	 *   __| |_   ____| |   (_)_ __  / _| ___
	 *  / _` \ \ / / _` |   | | '_ \| |_ / _ \
	 * | (_| |\ V / (_| |   | | | | |  _| (_) |
	 *  \__,_| \_/ \__,_|___|_|_| |_|_|  \___/
	 *                 |_____|
	 *
	 * ** display information about a DVD **
	 *
	 * dvd_info is a clone of lsdvd that adds additional features, such as
	 * JSON output, OGM chapter support, and checking for track errors.
	 *
	 */

int main(int argc, char **argv) {

	// Program name
	bool p_dvd_json = false;
	bool p_dvd_xchap = false;
	bool p_dvd_id = false;
	bool p_dvd_title = false;

	// lsdvd similar display output
	bool d_audio = false;
	bool d_video = false;
	bool d_chapters = false;
	bool d_subtitles = false;
	bool d_cells = false;

	// How much output
	bool debug = false;

	// limit results
	bool d_has_audio = false;
	bool d_has_subtitles = false;
	bool d_has_alang = false;
	bool d_has_slang = false;
	char d_alang[DVD_AUDIO_LANG_CODE + 1] = {'\0'};
	char d_slang[DVD_AUDIO_LANG_CODE + 1] = {'\0'};
	bool d_longest = false;
	bool opt_min_seconds = true;
	unsigned long int arg_number = 0;
	uint32_t arg_min_seconds = 0;
	bool opt_min_minutes = true;
	uint32_t arg_min_minutes = 0;
	bool opt_vts = false;
	uint16_t arg_vts = 0;
	bool d_is_valid = false;

	// dvd_info
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
	char device_filename[PATH_MAX];

	// libdvdread
	dvd_reader_t *dvdread_dvd = NULL;
	ifo_handle_t *vmg_ifo = NULL;
	ifo_handle_t *vts_ifo = NULL;

	// DVD
	struct dvd_info dvd_info;

	// Video Title Set
	struct dvd_vts dvd_vts[99];

	// Display formats
	const char *display_formats[4] = { "Pan and Scan or Letterbox", "Pan and Scan", "Letterbox", "Unset" };

	// getopt_long
	bool valid_args = true;
	bool opt_track_number = false;
	uint16_t arg_track_number = 0;
	int ix = 0;
	int opt = 0;
	bool invalid_opt = false;
	const char p_short_opts[] = "aAcdE:gG:hijlLM:N:sST:t:uVvxz";
	struct option p_long_opts[] = {

		{ "track", required_argument, NULL, 't' },
		{ "vts", required_argument, NULL, 'T' },

		{ "audio", no_argument, NULL, 'a' },
		{ "video", no_argument, NULL, 'v' },
		{ "chapters", no_argument, NULL, 'c' },
		{ "subtitles", no_argument, NULL, 's' },
		{ "cells", no_argument, NULL, 'd' },
		{ "all", no_argument, NULL, 'x' },

		{ "json", no_argument, NULL, 'j' },
		{ "xchap", no_argument, NULL, 'g' },
		{ "id", no_argument, NULL, 'i' },
		{ "volume", no_argument, NULL, 'u' },

		{ "longest", required_argument, NULL, 'l' },
		{ "min-seconds", required_argument, NULL, 'E' },
		{ "min-minutes", required_argument, NULL, 'M' },
		{ "has-audio", no_argument, NULL, 'A' },
		{ "has-alang", required_argument, NULL, 'N' },
		{ "has-subtitles", no_argument, NULL, 'S' },
		{ "has-slang", required_argument, NULL, 'G' },
		{ "valid", no_argument, NULL, 'L' },

		{ "help", no_argument, NULL, 'h' },
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

			case 'E':
				opt_min_seconds = true;
				arg_min_seconds = (uint32_t)strtoul(optarg, NULL, 10);
				break;

			case 'g':
				p_dvd_xchap = true;
				d_disc_title_header = false;
				d_chapters = true;
				break;

			case 'G':
				if(strlen(optarg) != 2 || !isalpha(optarg[0]) || !isalpha(optarg[1])) {
					fprintf(stderr, "Subtitle language code must be two characters\n");
					return 1;
				}
				strncpy(d_slang, optarg, DVD_AUDIO_LANG_CODE);
				d_has_slang = true;
				break;

			case 'i':
				p_dvd_id = true;
				break;

			case 'j':
				p_dvd_json = true;
				d_disc_title_header = false;
				d_audio = true;
				d_video = true;
				d_chapters = true;
				d_subtitles = true;
				d_cells = true;
				break;

			case 'l':
				d_longest = true;
				break;

			case 'L':
				d_is_valid = true;
				break;

			case 'M':
				opt_min_minutes = true;
				arg_min_minutes = (uint32_t)strtoul(optarg, NULL, 10);
				break;

			case 'N':
				if(strlen(optarg) != 2 || !isalpha(optarg[0]) || !isalpha(optarg[1])) {
					fprintf(stderr, "Audio language code must be two characters\n");
					return 1;
				}
				strncpy(d_alang, optarg, DVD_AUDIO_LANG_CODE);
				d_has_alang = true;
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

			case 'u':
				p_dvd_title = true;
				break;

			case 'v':
				d_video = true;
				break;

			case 'V':
				printf("dvd_info %s\n", PACKAGE_VERSION);
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
				printf("dvd_info - display information about a DVD\n");
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
				printf("  -l, --longest		Track with the longest length\n");
				printf("  -A, --has-audio       Track has audio streams\n");
				printf("  -N, --has-alang <xx>  Track has audio audio language, two character code\n");
				printf("  -S, --has-subtitles   Track has VobSub subtitles\n");
				printf("  -G, --has-slang <xx>  Track has subtitle language, two character code\n");
				printf("  -E, --seconds <secs>  Track has minimum number of seconds\n");
				printf("  -M, --minutes <mins>  Track has minimum number of minutes\n");
				printf("  -T, --vts <number>    Track is in video title set number\n");
				printf("  -L, --valid		Track is marked as valid\n");
				printf("\n");
				printf("Other:\n");
				printf("  -i, --id		Display DVD ID only\n");
				printf("  -u, --volume		Display DVD UDF volume name only (for ISO or disc)\n");
				printf("  -g, --xchap           Display title's chapter format for mkvmerge\n");
				printf("  -h, --help            Display these help options\n");
				printf("      --version         Display version\n");
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

	// If '-i /dev/device' is not passed, then set it to the string
	// passed.  fex: 'dvd_info /dev/dvd1' would change it from the default
	// of '/dev/dvd'.
	memset(device_filename, '\0', PATH_MAX);
	if (argv[optind])
		strncpy(device_filename, argv[optind], PATH_MAX - 1);
	else
		strncpy(device_filename, DEFAULT_DVD_DEVICE, PATH_MAX - 1);

	// Exit after all invalid input warnings have been sent
	if(valid_args == false)
		return 1;

	/** Begin dvd_info :) */

	dvdread_dvd = DVDOpen(device_filename);
	if(!dvdread_dvd) {
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

	dvd_info = dvd_info_open(dvdread_dvd, device_filename);
	if(dvd_info.valid == 0)
		return 1;

	// Display ID only if requested
	if(p_dvd_id) {
		printf("%s\n", dvd_info.dvdread_id);
		return 0;
	}

	// Display volume name only if requested
	if(p_dvd_title) {
		printf("%s\n", dvd_info.title);
		return 0;
	}

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

	ifo_handle_t *vts_ifos[DVD_MAX_VTS_IFOS];

	if(opt_vts && (arg_vts == 0 || arg_vts > dvd_info.video_title_sets)) {
		fprintf(stderr, "Video Title Set must be between 1 and %" PRIu16 "\n", dvd_info.video_title_sets);
		return 1;
	}

	uint8_t vts_ifo_ix;
	for(vts_ifo_ix = 0; vts_ifo_ix < 100; vts_ifo_ix++)
		vts_ifos[vts_ifo_ix] = NULL;

	// Do some checks to see if a VTS is ok or not
	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++)
		dvd_vts[vts] = dvd_vts_open(dvdread_dvd, vts);

	/**
	 * Track information
	 */

	struct dvd_track *dvd_tracks;
	dvd_tracks = dvd_tracks_init(dvdread_dvd, vmg_ifo, d_audio, d_subtitles, d_chapters, d_cells);

	dvd_info.longest_track = dvd_tracks[0].track;

	// Only display the longest track if requested
	if(d_longest) {
		d_first_track = dvd_info.longest_track;
		d_last_track = dvd_info.longest_track;
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
		printf("Tracks: %" PRIu16 ", ", dvd_info.tracks);
		printf("Longest track: %" PRIu16, dvd_info.longest_track);
		printf("\n");
	}

	/** dvdxchap display output **/
	if(p_dvd_xchap) {
		if(opt_track_number)
			dvd_xchap(dvd_tracks[arg_track_number]);
		else
			dvd_xchap(dvd_tracks[dvd_info.longest_track]);
		goto cleanup;
	}

	// Count valid, invalid tracks and title sets
	if(debug || opt_vts) {

		for(ix = 1; ix <= dvd_info.tracks; ix++) {
			dvd_vts[dvd_tracks[ix].vts].tracks++;
			if(dvd_tracks[ix].valid) {
				dvd_info.valid_tracks++;
				dvd_vts[dvd_tracks[ix].vts].valid_tracks++;
			} else {
				dvd_info.invalid_tracks++;
				dvd_vts[dvd_tracks[ix].vts].invalid_tracks++;
			}
		}

	}

	// Print the valid and invalid VTSs
	if(debug) {

		printf("        Tracks: %*" PRIu16 ", ", 2, dvd_info.tracks);
		printf("Valid: %*" PRIu16 ", ", 2, dvd_info.valid_tracks);
		printf("Invalid: %*" PRIu16, 2, dvd_info.invalid_tracks);
		printf("\n");

		for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {
			if(dvd_vts[vts].valid == true)
				dvd_info.valid_video_title_sets++;
			else
				dvd_info.invalid_video_title_sets++;
		}

		printf("        Video Title Sets: %*" PRIu16 ", ", 2, dvd_info.video_title_sets);
		printf("Valid: %*" PRIu16 ", ", 2, dvd_info.valid_video_title_sets);
		printf("Invalid: %*" PRIu16, 2, dvd_info.invalid_video_title_sets);
		printf("\n");

		for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {
			printf("        Title set: %*" PRIu16 ", ", 2, vts);
			printf("Tracks: %*" PRIu16 ", ", 2, dvd_vts[vts].tracks);
			printf("Valid: %*" PRIu16 ", ", 2, dvd_vts[vts].valid_tracks);
			printf("Invalid: %*" PRIu16, 2, dvd_vts[vts].invalid_tracks);
			printf("\n");
		}

	}

	// Display more specific Video Title Set information
	if(opt_vts) {
		printf("        Video Title Set: %*" PRIu16 ", ", 2, arg_vts);
		printf("Tracks: %*" PRIu16 ", ", 2, dvd_vts[arg_vts].tracks);
		printf("Valid tracks: %*" PRIu16 ", ", 2, dvd_vts[arg_vts].valid_tracks);
		printf("Invalid tracks: %*" PRIu16, 2, dvd_vts[arg_vts].invalid_tracks);
		printf("\n");
	}


	/** dvd_info output **/

	struct dvd_track dvd_track;
	struct dvd_video dvd_video;
	struct dvd_audio dvd_audio;
	struct dvd_subtitle dvd_subtitle;
	struct dvd_chapter dvd_chapter;
	struct dvd_cell dvd_cell;

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		dvd_track = dvd_tracks[track_number];
		dvd_video = dvd_tracks[track_number].dvd_video;

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

		// Skip if limiting to valid only
		if(d_is_valid && dvd_track.valid == false)
			continue;

		// Need an IFO if checking for audio or sub track languages
		if(d_has_alang || d_has_slang)
			vts_ifo = ifoOpen(dvdread_dvd, dvd_track.vts);

		// Skip if audio track language stream isn't found
		if(d_has_alang && !dvd_track_has_audio_lang_code(vts_ifo, d_alang))
			continue;

		// Skip if subtitle language stream isn't found
		if(d_has_slang && !dvd_track_has_subtitle_lang_code(vts_ifo, d_slang))
			continue;

		// Close out for next round
		if(vts_ifo) {
			ifoClose(vts_ifo);
			vts_ifo = NULL;
		}

		// Display track information
		printf("Track: %*" PRIu16 ", ", 2, dvd_track.track);
		printf("Length: %s, ", dvd_track.length);
		printf("Chapters: %*" PRIu8 ", ", 2, dvd_track.chapters);
		printf("Cells: %*" PRIu8 ", ", 2, dvd_track.cells);
		printf("Audio streams: %*" PRIu8 ", ", 2, dvd_track.audio_tracks);
		printf("Subpictures: %*" PRIu8 ", ", 2, dvd_track.subtitles);
		printf("Title set: %*" PRIu16", ", 2, dvd_track.vts);
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

				printf("        Audio: %*" PRIu8 ", ", 2, d_stream_num);
				printf("Language: %s, ", (strlen(dvd_audio.lang_code) ? dvd_audio.lang_code : "--"));
				printf("Codec: %s, ", dvd_audio.codec);
				printf("Channels: %" PRIu8 ", ", dvd_audio.channels);
				printf("Stream id: %s, ", dvd_audio.stream_id);
				printf("Active: %s", (dvd_audio.active ? "yes" : "no"));
				printf("\n");
				d_stream_num++;

			}

		}

		// Display subtitles
		if(d_subtitles && dvd_track.subtitles) {

			d_stream_num = 1;

			for(subtitle_track_ix = 0; subtitle_track_ix < dvd_track.subtitles; subtitle_track_ix++) {

				dvd_subtitle = dvd_track.dvd_subtitles[subtitle_track_ix];

				printf("        Subtitle: %*" PRIu8 ", ", 2, d_stream_num);
				printf("Language: %s, ", (strlen(dvd_subtitle.lang_code) ? dvd_subtitle.lang_code : "--"));
				printf("Stream id: %s, ", dvd_subtitle.stream_id);
				printf("Active: %s", (dvd_subtitle.active ? "yes" : "no"));
				printf("\n");

				d_stream_num++;

			}

		}

		// Display chapters
		if(d_chapters && dvd_track.chapters) {

			for(chapter_ix = 0; chapter_ix < dvd_track.chapters; chapter_ix++) {

				dvd_chapter = dvd_track.dvd_chapters[chapter_ix];
				printf("        Chapter: %*" PRIu8 ", ", 2, dvd_chapter.chapter);
				printf("Length: %s, ", dvd_chapter.length);
				printf("First cell: %*" PRIu8 ", ", 2, dvd_chapter.first_cell);
				printf("Last cell: %*" PRIu8 ", ", 2, dvd_chapter.last_cell);
				printf("Filesize: % 5.0lf MBs\n", dvd_chapter.filesize_mbs);

			}

		}

		// Display track cells
		if(d_cells && dvd_track.cells) {

			for(cell_ix = 0; cell_ix < dvd_track.cells; cell_ix++) {

				dvd_cell = dvd_track.dvd_cells[cell_ix];

				printf("        Cell: %*" PRIu8 ", ", 2, dvd_cell.cell);
				printf("Length: %s, ", dvd_cell.length);
				printf("First sector: %7" PRIu64 ", ", dvd_cell.first_sector);
				printf("Last sector: %7" PRIu64", ", dvd_cell.last_sector);
				printf("Filesize: % 5.0lf MBs", dvd_cell.filesize_mbs);
				printf("\n");

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
