#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <inttypes.h>
#include "dvd_info.h"
#include "dvd_device.h"
#include "dvd_vmg_ifo.h"
#include "dvd_vts.h"
#include "dvd_track.h"
#include "dvd_chapter.h"
#include "dvd_time.h"
#ifdef __linux__
#include <linux/cdrom.h>
#include "dvd_drive.h"
#endif
#ifndef VERSION
#define VERSION "1.0"
#endif

int main(int argc, char **argv);
void print_usage(char *binary);
void dvd_xchap(struct dvd_track dvd_track);

int main(int argc, char **argv) {

	// Program name
	char program_name[] = "dvd_xchap";

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

	// libdvdread
	dvd_reader_t *dvdread_dvd = NULL;
	ifo_handle_t *vmg_ifo = NULL;
	ifo_handle_t *vts_ifo = NULL;

	// DVD
	struct dvd_info dvd_info;
	memset(dvd_info.dvdread_id, '\0', sizeof(dvd_info.dvdread_id));
	dvd_info.video_title_sets = 1;
	dvd_info.tracks = 1;
	dvd_info.longest_track = 1;

	// Track
	struct dvd_track dvd_track;
	dvd_track.track = 1;
	dvd_track.valid = 1;
	dvd_track.vts = 1;
	dvd_track.ttn = 1;
	snprintf(dvd_track.length, DVD_TRACK_LENGTH + 1, "00:00:00.000");
	dvd_track.msecs = 0;
	dvd_track.chapters = 0;

	// Chapters
	struct dvd_chapter dvd_chapter;
	dvd_chapter.chapter = 0;
	snprintf(dvd_chapter.length, DVD_CHAPTER_LENGTH + 1, "00:00:00.000");

	// Statistics
	uint32_t longest_msecs = 0;

	// getopt_long
	bool valid_args = true;
	bool opt_track_number = false;
	unsigned int arg_track_number = 0;
	int ix = 0;
	int opt = 0;
	// Send 'invalid argument' to stderr
	opterr = 1;
	const char p_short_opts[] = "acdhjst:vxz";

	struct option p_long_opts[] = {

		{ "help", no_argument, NULL, 'h' },
		{ "track", required_argument, NULL, 't' },
		{ 0, 0, 0, 0 }

	};

	// parse options
	while((opt = getopt_long(argc, argv, p_short_opts, p_long_opts, &ix)) != -1) {

		// It's worth noting that if there are unknown options passed,
		// I just ignore them, and continue printing requested data.
		switch(opt) {

			case 'h':
				print_usage(program_name);
				return 0;

			case 't':
				opt_track_number = true;
				arg_track_number = (unsigned int)strtoumax(optarg, NULL, 0);
				break;

			case '?':
				print_usage(program_name);
				return 1;

			case 0:
			default:
				break;

		}

	}

	if (argv[optind])
		device_filename = argv[optind];
	else
		device_filename = DEFAULT_DVD_DEVICE;

	if(valid_args == false)
		return 1;

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

#ifdef __linux__

	// Poll drive status if it is hardware
	if(dvd_device_is_hardware(device_filename)) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "drive status: ");
			dvd_drive_display_status(device_filename);

			return 1;

		}

	}

#endif

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

	// Open VMG IFO -- where all the cool stuff is
	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "%s: Opening VMG IFO failed\n", program_name);
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Get the total number of title tracks on the DVD
	dvd_info.tracks = dvd_tracks(vmg_ifo);

	// Exit if track number requested does not exist
	if(opt_track_number && (arg_track_number > dvd_info.tracks || arg_track_number < 1)) {
		fprintf(stderr, "[%s] valid track numbers: 1 to %u\n", program_name, dvd_info.tracks);
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
	bool valid_ifos[DVD_MAX_VTS_IFOS] = { false };
	ifo_handle_t *vts_ifos[DVD_MAX_VTS_IFOS];
	vts_ifos[0] = NULL;

	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(vts_ifos[vts] == NULL) {
			valid_ifos[vts] = false;
			has_invalid_ifos = true;
			vts_ifos[vts] = NULL;
		} else if(!ifo_is_vts(vts_ifos[vts])) {
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
	}

	/**
	 * Track information
	 */

	struct dvd_track dvd_tracks[DVD_MAX_TRACKS];

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		// Open IFO
		dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, track_number);

		// Set track values to empty if it is invalid
		if(valid_ifos[dvd_track.vts] == false) {

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

		dvd_tracks[track_number - 1] = dvd_track;

	}

	/** Display DVD chapters **/
	track_number = (opt_track_number ? d_first_track : dvd_info.longest_track);
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

void print_usage(char *binary) {

	printf("%s %s - display information about a DVD\n", binary, VERSION);
	printf("\n");
	printf("Usage: %s [options] [-t track number] [dvd path]\n", binary);
	printf("\n");
	printf("Options:\n");
	printf("  -t, --track [number]	Display title track\n");

}

/**
 * Clone of dvdxchap from ogmtools, but with bug fixes! :)
 */
void dvd_xchap(struct dvd_track dvd_track) {

	// dvd_xchap format starts with a single chapter that begins
	// at zero milliseconds. Next, the only chapters that are displayed
	// are the ones that are not the final one, meaning there is no chapter
	// marker to jump to the very end of a file.
	// Therefore, at least 2 chapters must exist, since the last one is the
	// stopping point, and there needs to be something in the middle.

	printf("CHAPTER01=00:00:00.000\n");
	printf("CHAPTER01NAME=Chapter 01\n");

	if(dvd_track.valid && dvd_track.chapters > 1) {

		struct dvd_chapter dvd_chapter;
		uint32_t chapter_msecs = 0;
		uint8_t chapter_number = 1;
		char chapter_start[DVD_CHAPTER_LENGTH + 1] = {'\0'};
		uint8_t c = 0;

		for(c = 0; c < dvd_track.chapters - 1; c++) {

			// Offset indexing by two, since the first chapter here is the starting point of the video
			chapter_number = c + 2;

			dvd_chapter = dvd_track.dvd_chapters[c];

			chapter_msecs += dvd_chapter.msecs;
			strncpy(chapter_start, milliseconds_length_format(chapter_msecs), DVD_CHAPTER_LENGTH);

			printf("CHAPTER%02u=%s\n", chapter_number, chapter_start);
			printf("CHAPTER%02uNAME=Chapter %02u\n", chapter_number, chapter_number);

		}

	}

}
