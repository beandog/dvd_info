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
#include <dvdnav/dvdnav.h>
#include "dvd_info/dvd_device.h"
#include "dvd_info/dvd_drive.h"
#include "dvd_info/dvd_vmg_ifo.h"
#include "dvd_info/dvd_track.h"
#include "dvd_info/dvd_track_audio.h"
#include "dvd_info/dvd_track_subtitle.h"

#define DEFAULT_DVD_DEVICE "/dev/dvd"

void print_usage(char *binary);

void print_usage(char *binary) {

	printf("Usage %s [options] [-t track_number] [dvd path]\n", binary);
	printf("\n");

}

int main(int argc, char **argv) {

	// libdvdread
	int dvd_fd;
	dvd_reader_t *dvdread_dvd;
	ifo_handle_t *vmg_ifo;
	ifo_handle_t *track_ifo = NULL;
	unsigned char dvdread_id[16] = {'\0'};
	uint8_t vts_ttn;
	pgc_t *pgc;
	pgcit_t *vts_pgcit;

	// Device hardware
	char *device_filename = DEFAULT_DVD_DEVICE;
	int drive_status;
	__useconds_t sleepy_time = 1000000;
	int num_naps = 0;
	int max_num_naps = 60;
	bool is_hardware = false;
	bool is_image = false;

	// DVD
	uint16_t num_vts;
	uint16_t num_tracks;
	int dvd_disc_id;
	uint8_t dvd_disc_side;
	char title[33] = {'\0'};
	char provider_id[33] = {'\0'};
	bool has_provider_id = false;
	char vmg_id[13] = {'\0'};
	uint16_t longest_track;
	uint16_t longest_track_with_subtitles;
	uint16_t longest_16x9_track;
	uint16_t longest_4x3_track;
	uint16_t longest_letterbox_track;
	uint16_t longest_pan_scan_track;

	// Track
	int track_number = 0;
	int title_track_idx;
	uint8_t title_track_ifo_number;

	// Video
	char *video_codec;
	char *video_format;
	char *aspect_ratio;
	bool valid_video_codec = false;
	unsigned int video_height = 0;
	bool valid_video_format = false;
	bool valid_video_height = false;
	bool valid_aspect_ratio = true;
	unsigned int video_width = 720;
	bool valid_video_width = true;
	bool letterbox = false;

	// Audio
	uint8_t num_audio_streams;

	// Subtitles
	uint8_t subtitles;
	bool has_cc = false;
	bool has_cc_1 = false;
	bool has_cc_2 = false;

	// Originally 14, causes bug
	char title_track_length[13] = {'\0'};

	// getopt_long
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
	str_options = "hi:t:vz";

	struct option long_options[] = {

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
				track_number = atoi(optarg);
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
	if (argv[optind]) {
		device_filename = argv[optind];
	}

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
		// FIXME send to stderr
		printf("* Drive status: ");
		dvd_drive_display_status(device_filename);
	}

	// Wait for the drive to become ready
	// FIXME, make this optional?  Dunno.  Probably not.
	// At the very least, let the wait value be set. :)
	if(is_hardware) {
		if(!dvd_drive_has_media(device_filename)) {

			// FIXME send to stderr
			printf("drive status: ");
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
	num_tracks = dvd_info_num_tracks(vmg_ifo);

	// Exit if track number requested does not exist
	if(track_number > num_tracks || track_number < 1) {
		fprintf(stderr, "Invalid track number %d\n", track_number);
		fprintf(stderr, "Valid track numbers: 1 to %d\n", num_tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}


	// GRAB ALL THE THINGS
	// Total # of video title sets (or IFOs)
	num_vts = dvd_info_num_vts(vmg_ifo);
	dvd_disc_id = DVDDiscID(dvdread_dvd, dvdread_id);
	dvd_disc_side = vmg_ifo->vmgi_mat->disc_side;
	dvd_device_title(device_filename, title);
	dvd_info_provider_id(vmg_ifo, provider_id);
	dvd_info_vmg_id(vmg_ifo, vmg_id);
	longest_track = dvd_info_longest_track(dvdread_dvd);
	longest_track_with_subtitles = dvd_info_longest_track_with_subtitles(dvdread_dvd);
	longest_16x9_track = dvd_info_longest_16x9_track(dvdread_dvd);
	longest_4x3_track = dvd_info_longest_4x3_track(dvdread_dvd);
	longest_letterbox_track = dvd_info_longest_letterbox_track(dvdread_dvd);
	longest_pan_scan_track = dvd_info_longest_pan_scan_track(dvdread_dvd);

	printf("[DVD]\n");

	// Display starter information
	// # Video Title Sets (VTS) / IFOs
	printf("Total VTS: %d\n", num_vts);
	printf("Tracks: %d\n", num_tracks);

	// Display DVDDiscID from libdvdread
	if(dvd_disc_id == -1) {
		fprintf(stderr, "dvd_info: querying DVD id failed\n");
	} else {
		printf("Disc ID: ");
		for(unsigned long x = 0; x < sizeof(dvdread_id); x++) {
			printf("%02x", dvdread_id[x]);
		}
		printf("\n");
	}

	// DVD title
	printf("Title: ");
	printf("%s\n", title);

	// provider ID
	// Max length of provider ID is 32 letters, so create an array
	// that has enough size to store the letters and a null
	// terminator.  Also initialize it with all null terminators.

	// Having an empty provider ID is very common.
	if(provider_id[0] != '\0')
		has_provider_id = true;

	printf("Provider ID: ");
	printf("%s\n", provider_id);

	// VMG ID
	printf("VMG: ");
	printf("%s\n", vmg_id);

	// Disc side
	printf("Disc Side: ");
	printf("%i\n", dvd_disc_side);

	// Longest track number ordered by milliseconds
	printf("Longest track: ");
	printf("%i\n", longest_track);
	printf("Longest track with subtitles: ");
	printf("%i\n", longest_track_with_subtitles);
	printf("Longest 16x9 track: ");
	printf("%i\n", longest_16x9_track);
	printf("Longest 4x3 track: ");
	printf("%i\n", longest_4x3_track);
	printf("Longest letterbox track: ");
	printf("%i\n", longest_letterbox_track);
	printf("Longest pan & scan track: ");
	printf("%i\n", longest_pan_scan_track);

	/**
	 * Track information
	 */

	if(track_number) {

		printf("[Track %d]\n", track_number);

		title_track_idx = track_number - 1;
		title_track_ifo_number = dvd_track_ifo_number(vmg_ifo, track_number);
		track_ifo = ifoOpen(dvdread_dvd, title_track_ifo_number);
		vts_ttn = vmg_ifo->tt_srpt->title[title_track_idx].vts_ttn;

		printf("Video Title Set (IFO): %d\n", title_track_ifo_number);

		if(!track_ifo) {
			fprintf(stderr, "dvd_info: opening IFO %i failed\n", track_number);
			ifoClose(vmg_ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		if(!track_ifo->vtsi_mat) {
			printf("Could not open vtsi_mat for track %d\n", track_number);
			ifoClose(vmg_ifo);
			ifoClose(track_ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		vts_pgcit = track_ifo->vts_pgcit;
		pgc = vts_pgcit->pgci_srp[track_ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1].pgc;

		// Video codec
		if(dvd_track_mpeg1(track_ifo)) {
			video_codec = "MPEG1";
			valid_video_codec = true;
		} else if(dvd_track_mpeg2(track_ifo)) {
			video_codec = "MPEG2";
			valid_video_codec = true;
		} else {
			video_codec = "Unknown";
		}

		// Video format and height
		if(dvd_track_ntsc_video(track_ifo)) {
			video_format = "NTSC";
			video_height = 480;
			valid_video_format = true;
			valid_video_height = true;
		} else if(dvd_track_pal_video(track_ifo)) {
			video_format = "PAL";
			video_height = 576;
			valid_video_format = true;
			valid_video_height = true;
		} else {
			video_format = "Unknown";
		}

		// Aspect ratio
		if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
			aspect_ratio = "4:3";
		else if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
			aspect_ratio = "16:9";
		else {
			aspect_ratio = "Unknown";
			valid_aspect_ratio = false;
			fprintf(stderr, "Unknown aspect ratio: %i, expected 0 or 3\n", track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio);
			return 1;
		}

		// Video width
		if(track_ifo->vtsi_mat->vts_video_attr.picture_size == 0) {
			video_width = 720;
		} else if(track_ifo->vtsi_mat->vts_video_attr.picture_size == 1) {
			video_width = 704;
		} else if(track_ifo->vtsi_mat->vts_video_attr.picture_size == 2) {
			video_width = 352;
		} else if(track_ifo->vtsi_mat->vts_video_attr.picture_size == 3) {
			video_width = 352;
			if(video_height)
				video_height = video_height / 2;
		} else {
			valid_video_width = false;
			fprintf(stderr, "Invalid video width: %i\n", track_ifo->vtsi_mat->vts_video_attr.picture_size);
			return 1;
		}

		// Letterbox
		letterbox = dvd_track_letterbox_video(track_ifo);

		// Closed Captioning
		if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_1 || track_ifo->vtsi_mat->vts_video_attr.line21_cc_2) {
			has_cc = true;
			if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_1)
				has_cc_1 = true;
			if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_2)
				has_cc_2 = true;
		}

		// Audio streams
		num_audio_streams = dvd_track_num_audio_streams(track_ifo);

		// Subtitles
		subtitles = dvd_track_subtitles(track_ifo);

		// Video codec
		printf("Video Codec: ");
		printf("%s\n", video_codec);

		// Video format
		printf("Video Format: ");
		printf("%s\n", video_format);

		// Aspect ratio
		printf("Aspect Ratio: ");
		printf("%s\n", aspect_ratio);

		// Video width
		printf("Video Width: ");
		printf("%i\n", video_width);

		// Video height
		printf("Video Height: ");
		printf("%i\n", video_height);

		// Letterbox
		printf("Letterbox: ");
		if(letterbox)
			printf("1\n");
		else
			printf("0\n");

		// Closed Captioning
		// Not sure if this is right or not
		/*
		printf("closed captioning: ");
		printf("1\n");
		*/

		// Display number of subtitles
		printf("Subtitles: ");
		printf("%i\n", subtitles);

		// Title track length (HH:MM:SS.MS)
		dvd_track_str_length(&pgc->playback_time, title_track_length);

		printf("Length: ");
		printf("%s\n", title_track_length);

		// Audio streams
		printf("Audio Streams: ");
		printf("%i\n", num_audio_streams);

		/** Audio Streams **/

		char lang_code[3] = {'\0'};
		char audio_codec[5] = {'\0'};
		int audio_channels;
		int audio_stream_id;
		uint8_t stream_idx;

		for(stream_idx = 0; stream_idx < num_audio_streams; stream_idx++) {
			printf("[Audio Track %i:%i]\n", track_number, stream_idx + 1);

			dvd_track_audio_lang_code(track_ifo, stream_idx, lang_code);
			printf("Language Code: %s\n", lang_code);

			dvd_track_audio_codec(track_ifo, stream_idx, audio_codec);
			printf("Audio Codec: %s\n", audio_codec);

			audio_channels = dvd_track_audio_num_channels(track_ifo, stream_idx);
			printf("Channels: %i\n", audio_channels);

			audio_stream_id = dvd_track_audio_stream_id(track_ifo, stream_idx);
			printf("Stream ID: %i\n", audio_stream_id);
		}

		for(stream_idx = 0; stream_idx < subtitles; stream_idx++) {

			printf("[Subtitle Track %i:%i]\n", track_number, stream_idx + 1);

			dvd_track_subtitle_lang_code(track_ifo, stream_idx, lang_code);
			printf("Language Code: %s\n", lang_code);

		}

	}

	// Cleanup

	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(track_ifo)
		ifoClose(track_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	/*
	if(dvdnav_dvd)
		dvdnav_close(dvdnav_dvd);
	*/

	return 0;

}
