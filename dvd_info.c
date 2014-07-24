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
#include "dvd_device.h"
#include "dvd_drive.h"
#include "dvd_vmg_ifo.h"
#include "dvd_track.h"
#include "dvd_track_audio.h"
#include "dvd_track_subtitle.h"

#define DEFAULT_DVD_DEVICE "/dev/dvd"

void print_usage(char *binary);

void print_usage(char *binary) {

	printf("Usage: %s [options] [-t track number] [dvd path]\n", binary);

}

struct dvd_info {
	uint16_t video_title_sets;
	uint8_t side;
	char title[33];
	char provider_id[33];
	char vmg_id[13];
	uint16_t tracks;
	uint16_t longest_track;
};

struct dvd_track {
	int number;
	int title_idx;
	uint8_t vts;
	char length[13];
	uint8_t chapters;
	uint8_t audio_tracks;
	uint8_t subtitles;
};

struct dvd_video {
	char codec[6];
	char format[5];
	char *aspect_ratio;
	uint16_t width;
	uint16_t height;
	bool letterbox;
	bool pan_and_scan;
	bool closed_captioning[3];
};

struct dvd_audio {
	int track;
	int stream;
	char lang_code[3];
	char codec[5];
	int channels;
};

struct dvd_subtitle {
	int track;
	int stream;
	char lang_code[3];
};

int main(int argc, char **argv) {

	// Device hardware
	int dvd_fd;
	char *device_filename = DEFAULT_DVD_DEVICE;
	int drive_status;
	__useconds_t sleepy_time = 1000000;
	int num_naps = 0;
	int max_num_naps = 60;
	bool is_hardware = false;
	bool is_image = false;

	// libdvdread
	dvd_reader_t *dvdread_dvd;
	ifo_handle_t *vmg_ifo = NULL;
	ifo_handle_t *track_ifo = NULL;
	unsigned char dvdread_ifo_md5[16] = {'\0'};
	char dvdread_id[33] = {'\0'};
	uint8_t vts_ttn;
	pgc_t *pgc;
	pgcit_t *vts_pgcit;
	int dvdread_retval;

	// DVD
	struct dvd_info dvd_info;
	memset(dvd_info.title, '\0', 33);
	memset(dvd_info.provider_id, '\0', 33);
	memset(dvd_info.vmg_id, '\0', 13);

	// Track
	struct dvd_track dvd_track;
	dvd_track.number = 0;
	dvd_track.title_idx = 0;
	dvd_track.vts = 1;
	memset(dvd_track.length, '\0', 13);
	dvd_track.chapters = 1;
	dvd_track.audio_tracks = 0;
	dvd_track.subtitles = 0;

	// Video
	struct dvd_video dvd_video;
	memset(dvd_video.codec, '\0', 6);
	memset(dvd_video.format, '\0', 5);
	dvd_video.height = 0;
	dvd_video.width = 0;
	dvd_video.letterbox = false;
	dvd_video.pan_and_scan = false;
	dvd_video.closed_captioning[0] = false;
	dvd_video.closed_captioning[1] = false;
	dvd_video.closed_captioning[2] = false;

	// Audio
	struct dvd_audio dvd_audio;
	uint8_t stream;
	dvd_audio.track = 1;
	dvd_audio.stream = 0;
	memset(dvd_audio.lang_code, '\0', 3);
	memset(dvd_audio.codec, '\0', 5);

	// Subtitles
	struct dvd_subtitle dvd_subtitle;
	dvd_subtitle.track = 1;
	dvd_subtitle.stream = 0;
	memset(dvd_subtitle.lang_code, '\0', 3);

	// getopt_long
	int track_number = 0;
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
	if(track_number > dvd_info.tracks || track_number < 0) {
		fprintf(stderr, "Invalid track number %d\n", track_number);
		fprintf(stderr, "Valid track numbers: 1 to %d\n", dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Exit if we cannot get libdvdread DiscID
	dvdread_retval = DVDDiscID(dvdread_dvd, dvdread_ifo_md5);
	if(dvdread_retval == -1) {
		fprintf(stderr, "Querying DVD id failed -- this is probably related to the library not being able to open an IFO.  Check the DVD for physical defects.\n");
		return 1;
	}

	// GRAB ALL THE THINGS
	// Total # of video title sets (or IFOs)
	dvd_info.video_title_sets = dvd_info_num_vts(vmg_ifo);
	dvd_info.side = vmg_ifo->vmgi_mat->disc_side;
	dvd_device_title(device_filename, dvd_info.title);
	dvd_info_provider_id(vmg_ifo, dvd_info.provider_id);
	dvd_info_vmg_id(vmg_ifo, dvd_info.vmg_id);
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
	for(unsigned long x = 0; x < 16; x++) {
		sprintf(&dvdread_id[x * 2], "%02x", dvdread_ifo_md5[x]);
	}

	printf("[DVD]\n");
	printf("Title: %s\n", dvd_info.title);
	printf("Provider ID: %s\n", dvd_info.provider_id);
	printf("VMG: %s\n", dvd_info.vmg_id);
	printf("Disc Side: %i\n", dvd_info.side);
	printf("VTS: %d\n", dvd_info.video_title_sets);
	printf("Tracks: %d\n", dvd_info.tracks);
	printf("Longest track: %i\n", dvd_info.longest_track);
	/**
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
	*/

	// libdvdread MD5 hash of the first 10 IFOs
	printf("dvdread id: %s\n", dvdread_id);

	/**
	 * Track information
	 */

	if(track_number) {

		// Open IFO
		track_ifo = ifoOpen(dvdread_dvd, dvd_track.vts);
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

		dvd_track.number = track_number;
		dvd_track.title_idx = track_number - 1;
		dvd_track.vts = dvd_track_ifo_number(vmg_ifo, track_number);
		vts_ttn = vmg_ifo->tt_srpt->title[dvd_track.title_idx].vts_ttn;
		vts_pgcit = track_ifo->vts_pgcit;
		pgc = vts_pgcit->pgci_srp[track_ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1].pgc;
		dvd_track_str_length(&pgc->playback_time, dvd_track.length);
		dvd_track.chapters = pgc->nr_of_programs;
		dvd_track_video_codec(track_ifo, dvd_video.codec);
		dvd_track_video_format(track_ifo, dvd_video.format);
		dvd_video.width = dvd_track_video_width(track_ifo);
		dvd_video.height = dvd_track_video_height(track_ifo);

		// Aspect ratio
		if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
			dvd_video.aspect_ratio = "4:3";
		else if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
			dvd_video.aspect_ratio = "16:9";
		else {
			// Catcch wrong integer value burned into DVD
			dvd_video.aspect_ratio = "Unknown";
		}

		// Letterbox
		dvd_video.letterbox = dvd_track_letterbox_video(track_ifo);

		// Pan & Scan
		dvd_video.pan_and_scan = dvd_track_pan_scan_video(track_ifo);

		// Closed Captioning
		if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_1 || track_ifo->vtsi_mat->vts_video_attr.line21_cc_2) {
			dvd_video.closed_captioning[0] = true;
			if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_1)
				dvd_video.closed_captioning[1] = true;
			if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_2)
				dvd_video.closed_captioning[2] = true;
		}

		// Audio streams
		dvd_track.audio_tracks = dvd_track_num_audio_streams(track_ifo);

		// Subtitles
		dvd_track.subtitles = dvd_track_subtitles(track_ifo);

		// Video codec
		printf("[Track %d]\n", track_number);
		printf("Video Title Set (IFO): %d\n", dvd_track.vts);
		printf("Chapters: %i\n", dvd_track.chapters);
		printf("Video Codec: %s\n", dvd_video.codec);
		printf("Video Format: %s\n", dvd_video.format);
		printf("Aspect Ratio: %s\n", dvd_video.aspect_ratio);
		printf("Video Width: %i\n", dvd_video.width);
		printf("Video Height: %i\n", dvd_video.height);
		printf("Letterbox: %i\n", dvd_video.letterbox ? 1 : 0);
		printf("Pan & Scan: %i\n", dvd_video.pan_and_scan ? 1 : 0);
		printf("Subtitles: %i\n", dvd_track.subtitles);
		printf("Length: %s\n", dvd_track.length);

		// Audio streams
		printf("Audio Streams: %i\n", dvd_track.audio_tracks);

		/** Audio Streams **/

		for(stream = 0; stream < dvd_track.audio_tracks; stream++) {

			memset(&dvd_audio, 0, sizeof(dvd_audio));

			dvd_audio.track = stream + 1;
			dvd_audio.stream = stream;
			dvd_track_audio_lang_code(track_ifo, stream, dvd_audio.lang_code);
			dvd_track_audio_codec(track_ifo, stream, dvd_audio.codec);
			dvd_audio.channels = dvd_track_audio_num_channels(track_ifo, stream);
			dvd_audio.stream = dvd_track_audio_stream_id(track_ifo, stream);

			printf("[Audio Track %i:%i]\n", track_number, stream + 1);
			printf("Language Code: %s\n", dvd_audio.lang_code);
			printf("Audio Codec: %s\n", dvd_audio.codec);
			printf("Channels: %i\n", dvd_audio.channels);
			printf("Stream ID: 0x%x\n", dvd_audio.stream);

		}

		for(stream = 0; stream < dvd_track.subtitles; stream++) {

			memset(&dvd_subtitle, 0, sizeof(dvd_subtitle));

			dvd_subtitle.track = stream + 1;
			dvd_subtitle.stream = dvd_track_subtitle_stream_id(stream);
			dvd_track_subtitle_lang_code(track_ifo, stream, dvd_subtitle.lang_code);

			printf("[Subtitle Track %i:%i]\n", track_number, stream + 1);
			printf("Stream ID: 0x%x\n", dvd_subtitle.stream);
			printf("Language Code: %s\n", dvd_subtitle.lang_code);

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
