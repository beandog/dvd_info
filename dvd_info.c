#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <inttypes.h>
#include <linux/cdrom.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/dvd_udf.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>
#include <jansson.h>
#include "dvd_info.h"
#include "dvd_device.h"
#include "dvd_drive.h"
#include "dvd_vmg_ifo.h"
#include "dvd_track.h"

void print_usage(char *binary);

void print_usage(char *binary) {

	printf("Usage: %s [options] [-t track number] [dvd path]\n", binary);

}

struct dvd_info {
	uint16_t video_title_sets;
	uint8_t side;
	char title[DVD_TITLE + 1];
	char provider_id[DVD_PROVIDER_ID + 1];
	char vmg_id[DVD_VMG_ID + 1];
	uint16_t tracks;
	uint16_t longest_track;
};

struct dvd_track {
	uint16_t ix;
	uint16_t vts;
	uint8_t ttn;
	char vts_id[DVD_TRACK_VTS_ID + 1];
	char length[DVD_TRACK_LENGTH + 1];
	uint32_t msecs;
	uint8_t chapters;
	uint8_t audio_tracks;
	uint8_t subtitles;
	uint8_t cells;
};

struct dvd_video {
	char codec[DVD_VIDEO_CODEC + 1];
	char format[DVD_VIDEO_FORMAT + 1];
	char aspect_ratio[DVD_VIDEO_ASPECT_RATIO + 1];
	uint16_t width;
	uint16_t height;
	bool letterbox;
	bool pan_and_scan;
	uint8_t df;
	char fps[DVD_VIDEO_FPS + 1];
	uint8_t angles;
};

struct dvd_audio {
	uint8_t ix;
	char stream_id[DVD_AUDIO_STREAM_ID + 1];
	char lang_code[DVD_AUDIO_LANG_CODE + 1];
	char codec[DVD_AUDIO_CODEC + 1];
	uint8_t channels;
};

struct dvd_subtitle {
	uint8_t ix;
	char stream_id[DVD_SUBTITLE_STREAM_ID + 1];
	char lang_code[DVD_SUBTITLE_LANG_CODE + 1];
};

struct dvd_chapter {
	uint8_t ix;
	char length[DVD_CHAPTER_LENGTH + 1];
};

struct dvd_cell {
	uint8_t ix;
	char length[DVD_CELL_LENGTH + 1];
};

int main(int argc, char **argv) {

	// Display output
	int d_lsdvd = 1;
	int verbose = 0;

	// dvd_info
	bool d_all_tracks;
	uint16_t d_first_track = 0;
	uint16_t d_last_track = 0;
	uint16_t track_number = 0;
	uint16_t total_tracks;
	uint16_t vts = 1;
	bool has_invalid_ifos = false;

	// Device hardware
	int dvd_fd;
	char *device_filename = DEFAULT_DVD_DEVICE;
	int drive_status;
	__useconds_t sleepy_time = 1000000;
	uint8_t num_naps = 0;
	uint8_t max_num_naps = 60;
	bool is_hardware = false;
	bool is_image = false;

	// libdvdread
	dvd_reader_t *dvdread_dvd;
	ifo_handle_t *vmg_ifo = NULL;
	ifo_handle_t *vts_ifo = NULL;
	ifo_handle_t *track_ifo = NULL;
	uint8_t dvdread_ifo_md5[16] = {'\0'};
	char dvdread_id[DVD_DVDREAD_ID + 1] = {'\0'};
	pgc_t *pgc;
	pgcit_t *vts_pgcit;
	int dvdread_retval;

	// DVD
	struct dvd_info dvd_info;
	dvd_info.video_title_sets = 1;
	dvd_info.side = 1;
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	memset(dvd_info.provider_id, '\0', sizeof(dvd_info.provider_id));
	memset(dvd_info.vmg_id, '\0', sizeof(dvd_info.vmg_id));
	dvd_info.tracks = 1;
	dvd_info.longest_track = 1;

	// Track
	struct dvd_track dvd_track;
	dvd_track.ix = 1;
	dvd_track.vts = 1;
	dvd_track.ttn = 1;
	memset(dvd_track.vts_id, '\0', sizeof(dvd_track.vts_id));
	memset(dvd_track.length, '\0', sizeof(dvd_track.length));
	dvd_track.msecs = 0;
	dvd_track.chapters = 1;
	dvd_track.audio_tracks = 0;
	dvd_track.subtitles = 0;
	dvd_track.cells = 1;

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
	uint8_t audio_stream;
	audio_stream = 0;
	dvd_audio.ix = 1;
	memset(dvd_audio.stream_id, '\0', sizeof(dvd_audio.stream_id));
	memset(dvd_audio.lang_code, '\0', sizeof(dvd_audio.lang_code));
	memset(dvd_audio.codec, '\0', sizeof(dvd_audio.codec));
	dvd_audio.channels = 0;

	// Subtitles
	struct dvd_subtitle dvd_subtitle;
	uint8_t subtitle_stream;
	subtitle_stream = 0;
	dvd_subtitle.ix = 1;
	memset(dvd_subtitle.stream_id, '\0', sizeof(dvd_subtitle.stream_id));
	memset(dvd_subtitle.lang_code, '\0', sizeof(dvd_subtitle.lang_code));

	// Chapters
	struct dvd_chapter dvd_chapter;
	dvd_chapter.ix = 0;
	memset(dvd_chapter.length, '\0', sizeof(dvd_chapter.length));

	// Cells
	struct dvd_cell dvd_cell;
	dvd_cell.ix = 0;
	memset(dvd_cell.length, '\0', sizeof(dvd_cell.length));

	// getopt_long
	bool opt_track_number = false;
	int arg_track_number = 0;
	int long_index = 0;
	int opt;
	// Send 'invalid argument' to stderr
	opterr= 1;
	// Check for invalid input
	bool valid_args = true;
	// Not enabled by an argument, set internally
	// I could probably come up with a better variable name. I probably would if
	// I understood getopt better. :T
	char *str_options;
	str_options = "hi:t:v";

	struct option long_options[] = {

		{ "verbose", no_argument, & verbose, 1 },

		// Entries with both a name and a value, will take either the
		// long option or the short one.  Fex, '--device' or '-i'
		{ "device", required_argument, 0, 'i' },
		{ "track", required_argument, 0, 't' },
		{ 0, 0, 0, 0 }

	};

	// parse options
	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {

		// It's worth noting that if there are unknown options passed,
		// I just ignore them, and continue printing requested data.
		switch(opt) {

			case 'h':
				print_usage(argv[0]);
				return 0;

			case 'i':
				device_filename = optarg;
				break;

			case 't':
				opt_track_number = true;
				arg_track_number = atoi(optarg);
				break;

			case 'v':
				verbose = 1;
				break;

			// ignore unknown arguments
			case '?':
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

	// Exit after all invalid input warnings have been sent
	if(valid_args == false)
		return 1;

	/** Begin dvd_info :) */

	// Check to see if device can be accessed
	if(!dvd_device_access(device_filename)) {
		fprintf(stderr, "cannot access %s\n", device_filename);
		return 1;
	}

	// Check to see if device can be opened
	dvd_fd = dvd_device_open(device_filename);
	if(dvd_fd < 0) {
		fprintf(stderr, "error opening %s\n", device_filename);
		return 1;
	}
	dvd_device_close(dvd_fd);

	// Check if it is hardware or an image file
	is_hardware = dvd_device_is_hardware(device_filename);
	is_image = dvd_device_is_image(device_filename);

	// Poll drive status if it is hardware
	if(is_hardware) {

		drive_status = dvd_drive_get_status(device_filename);

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "drive status: ");
			dvd_drive_display_status(device_filename);

			fprintf(stderr, "dvd_info: waiting for media\n");
			fprintf(stderr, "dvd_info: will give up after one minute\n");
			while(!dvd_drive_has_media(device_filename) && (num_naps < max_num_naps)) {
				usleep(sleepy_time);
				num_naps = num_naps + 1;

				// This is slightly annoying, even for me.
				fprintf(stderr, "%i ", num_naps);

				// Tired of waiting, exiting out
				if(num_naps == max_num_naps) {
					fprintf(stderr, "\n");
					fprintf(stderr, "dvd_info: tired of waiting for media, quitting\n");
					return 1;
				}

			}

		}

	}

	// begin libdvdread usage

	// Open DVD device
	dvdread_dvd = DVDOpen(device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "Opening DVD %s failed\n", device_filename);
		return 1;
	}

	// Open VMG IFO -- where all the cool stuff is
	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	if(!vmg_ifo) {
		fprintf(stderr, "Opening VMG IFO failed\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Get the total number of title tracks on the DVD
	dvd_info.tracks = dvd_info_num_tracks(vmg_ifo);

	// Exit if track number requested does not exist
	if(opt_track_number && (arg_track_number > dvd_info.tracks || arg_track_number < 1)) {
		fprintf(stderr, "Invalid track number %d\n", arg_track_number);
		fprintf(stderr, "Valid track numbers: 1 to %u\n", dvd_info.tracks);
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

	total_tracks = d_last_track - d_first_track + 1;

	// Exit if all the IFOs cannot be opened
	dvd_info.video_title_sets = dvd_info_num_vts(vmg_ifo);
	bool valid_ifos[dvd_info.video_title_sets];
	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		vts_ifo = ifoOpen(dvdread_dvd, vts);

		if(!vts_ifo) {
			fprintf(stderr, "Opening VTS IFO %u failed!\n", vts);
			valid_ifos[vts] = false;
			has_invalid_ifos = true;
		} else if(!vts_ifo->vtsi_mat) {
			printf("Could not open VTSI_MAT for VTS IFO %u\n", vts);
			valid_ifos[vts] = false;
			has_invalid_ifos = true;
		} else {
			valid_ifos[vts] = true;
			ifoClose(vts_ifo);
			vts_ifo = NULL;
		}

	}

	// Exit if the track requested is on an invalid IFO
	if(has_invalid_ifos && opt_track_number) {

		vts = dvd_track_ifo_number(vmg_ifo, track_number);

		if(valid_ifos[vts] == false) {

			fprintf(stderr, "Could not open IFO %u for track %u, exiting\n", vts, track_number);
			ifoClose(vmg_ifo);
			DVDClose(dvdread_dvd);
			return 1;

		}

	}

	// Exit if we cannot get libdvdread DiscID
	dvdread_retval = DVDDiscID(dvdread_dvd, dvdread_ifo_md5);
	if(dvdread_retval == -1) {
		fprintf(stderr, "Querying DVD id failed -- this is probably related to the library not being able to open an IFO.  Check the DVD for physical defects.\n");
		return 1;
	}

	// GRAB ALL THE THINGS
	dvd_info.side = vmg_ifo->vmgi_mat->disc_side;
	dvd_device_title(device_filename, dvd_info.title);
	strncpy(dvd_info.provider_id, dvd_info_provider_id(vmg_ifo), DVD_PROVIDER_ID);
	strncpy(dvd_info.vmg_id, dvd_info_vmg_id(vmg_ifo), DVD_VMG_ID);
	dvd_info.longest_track = dvd_info_longest_track(dvdread_dvd);

	/*
	longest_track_with_subtitles = dvd_info_longest_track_with_subtitles(dvdread_dvd);
	longest_16x9_track = dvd_info_longest_16x9_track(dvdread_dvd);
	longest_4x3_track = dvd_info_longest_4x3_track(dvdread_dvd);
	longest_letterbox_track = dvd_info_longest_letterbox_track(dvdread_dvd);
	longest_pan_scan_track = dvd_info_longest_pan_scan_track(dvdread_dvd);
	*/
	// libdvdread DVDDiscID()
	// Convert hex values to a string
	for(unsigned long x = 0; x < (DVD_DVDREAD_ID / 2); x++) {
		sprintf(&dvdread_id[x * 2], "%02x", dvdread_ifo_md5[x]);
	}

	/**
	 * Track information
	 */

	struct dvd_track dvd_tracks[total_tracks];

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		// Open IFO
		dvd_track.vts = dvd_track_ifo_number(vmg_ifo, track_number);

		// Skip track if parent IFO is invalid
		if(valid_ifos[dvd_track.vts] == false) {
			fprintf(stderr, "IFO %u for track %u is invalid, skipping track\n", dvd_track.vts, track_number);
			break;
		}

		track_ifo = ifoOpen(dvdread_dvd, dvd_track.vts);

		dvd_track.ix = track_number;
		dvd_track.ttn = vmg_ifo->tt_srpt->title[dvd_track.ix - 1].vts_ttn;
		strncpy(dvd_track.vts_id, dvd_track_vts_id(track_ifo), DVD_TRACK_VTS_ID);
		vts_pgcit = track_ifo->vts_pgcit;
		pgc = vts_pgcit->pgci_srp[track_ifo->vts_ptt_srpt->title[dvd_track.ttn - 1].ptt[0].pgcn - 1].pgc;
		strncpy(dvd_track.length, dvd_track_str_length(&pgc->playback_time), DVD_TRACK_LENGTH);
		dvd_track.msecs = dvd_track_length(&pgc->playback_time);
		dvd_track.chapters = pgc->nr_of_programs;
		strncpy(dvd_video.codec, dvd_track_video_codec(track_ifo), DVD_VIDEO_CODEC);
		strncpy(dvd_video.format, dvd_track_video_format(track_ifo), DVD_VIDEO_FORMAT);
		dvd_video.width = dvd_track_video_width(track_ifo);
		dvd_video.height = dvd_track_video_height(track_ifo);
		strncpy(dvd_video.aspect_ratio, dvd_track_video_aspect_ratio(track_ifo), DVD_VIDEO_ASPECT_RATIO);
		dvd_video.letterbox = dvd_track_letterbox_video(track_ifo);
		dvd_video.pan_and_scan = dvd_track_pan_scan_video(track_ifo);
		dvd_video.df = dvd_track_permitted_df(track_ifo);
		dvd_video.angles = dvd_track_angles(vmg_ifo, track_number);
		dvd_track.audio_tracks = dvd_track_num_audio_streams(track_ifo);
		dvd_track.subtitles = dvd_track_subtitles(track_ifo);
		dvd_track.cells = pgc->nr_of_cells;
		strncpy(dvd_video.fps, dvd_track_str_fps(&pgc->playback_time), DVD_VIDEO_FPS);

		/** Audio Streams **/

		for(audio_stream = 0; audio_stream < dvd_track.audio_tracks; audio_stream++) {

			memset(&dvd_audio, 0, sizeof(dvd_audio));
			memset(dvd_audio.lang_code, '\0', sizeof(dvd_audio.lang_code));
			memset(dvd_audio.codec, '\0', sizeof(dvd_audio.codec));

			dvd_audio.ix = audio_stream + 1;
			strncpy(dvd_audio.lang_code, dvd_track_audio_lang_code(track_ifo, audio_stream), DVD_AUDIO_LANG_CODE);
			strncpy(dvd_audio.codec, dvd_track_audio_codec(track_ifo, audio_stream), DVD_AUDIO_CODEC);
			dvd_audio.channels = dvd_track_audio_num_channels(track_ifo, audio_stream);
			strncpy(dvd_audio.stream_id, dvd_track_audio_stream_id(track_ifo, audio_stream), DVD_AUDIO_STREAM_ID);

		}

		/** Subtitles **/

		for(subtitle_stream = 0; subtitle_stream < dvd_track.subtitles; subtitle_stream++) {

			memset(&dvd_subtitle, 0, sizeof(dvd_subtitle));
			memset(dvd_subtitle.lang_code, '\0', sizeof(dvd_subtitle.lang_code));

			dvd_subtitle.ix = subtitle_stream + 1;
			strncpy(dvd_subtitle.stream_id, dvd_track_subtitle_stream_id(subtitle_stream), DVD_SUBTITLE_STREAM_ID);
			strncpy(dvd_subtitle.lang_code, dvd_track_subtitle_lang_code(track_ifo, subtitle_stream), DVD_SUBTITLE_LANG_CODE);

		}

		/** Chapters **/

		for(dvd_chapter.ix = 1; dvd_chapter.ix < dvd_track.chapters + 1; dvd_chapter.ix++) {

			strncpy(dvd_chapter.length, dvd_track_str_chapter_length(pgc, dvd_chapter.ix), DVD_CHAPTER_LENGTH);

		};

		memmove(&dvd_tracks[track_number - 1], &dvd_track, sizeof(dvd_track));

	}


	// Display output in lsdvd compatability mode
	struct dvd_track *track;

	if(d_lsdvd == 1)
		printf("Disc Title: %s\n", dvd_info.title);

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		track = &dvd_tracks[track_number - 1];

		if(d_lsdvd == 1) {

			printf("Title: %02u, ", track->ix);
			printf("Length: %s ", track->length);
			printf("Chapters: %02u, ", track->chapters);
			printf("Cells: %02u, ", track->cells);
			printf("Audio streams: %02u, ", track->audio_tracks);
			printf("Subpictures: %02u\n", track->subtitles);

		}

	}

	if(d_lsdvd && d_all_tracks)
		printf("Longest track: %02u\n", dvd_info.longest_track);

	// Cleanup

	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(track_ifo)
		ifoClose(track_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	return 0;

}
