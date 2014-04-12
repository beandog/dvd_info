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
#include <dvdnav/dvdnav.h>
#include "dvd_device.h"
#include "dvd_drive.h"
#include "dvd_info.h"

/**
 * Output on 'dvd_info -h'
 */
void print_usage(char *binary) {

	printf("Usage %s [options] [-t track_number] [dvd path]\n", binary);
	printf("\n");
	printf("Display DVD info:\n");
	printf("  --all			Display all\n");
	printf("  --id			Unique DVD identifier\n");
	printf("  --title		DVD title\n");
	printf("  --num-tracks		Number of tracks\n");
	printf("  --num-vts		Number of VTSs\n");
	printf("  --provider-id 	Provider ID\n");
	printf("  --serial-id 		Serial ID\n");
	printf("  --vmg-id		VMG ID\n");
	printf("  --side		Disc side\n");
	printf("\n");
	printf("Display track info:\n");
	printf("  --video-codec		Video codec (MPEG1 / MPEG2)\n");
	printf("  --video-format	Video format (NTSC / PAL )\n");
	printf("  --aspect-ratio	Aspect ratio (16:9, 4:3)\n");
	printf("  --video-width		Video width (480, 576, 288)\n");
	printf("  --video-height	Video height (720, 704, 352)\n");
	printf("  --letterbox		Letterbox video (0 [no], 1 [yes])\n");
	printf("  --film-mode		Film mode (film [movie], video [camera])\n");
	printf("  --num-audio-streams	Number of audio streams\n");
	printf("  --num-subtitles	Number of VOBSUB subtitles\n");
	printf("\n");
	printf("Display subtitle info:\n");
	printf("  --cc			Closed captioning (0 [no], 1 [yes])\n");

}

int dvd_track_video_codec(ifo_handle_t *track_ifo, char *video_codec) {

	char *codec;

	if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
		codec = "MPEG1";
	else if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		codec = "MPEG2";

	strncpy(video_codec, codec, 6);

	return 0;

}

int main(int argc, char **argv) {

	// DVD file descriptor
	int dvd_fd;

	// DVD drive status
	int drive_status;

	// DVD track number -- default to 0, which basically means, ignore me.
	unsigned int track_number = 0;

	// Do a check to see if the DVD filename given is hardware ('/dev/foo')
	bool is_hardware = false;
	// Or if it's an image file, filename (ISO, UDF, etc.)
	bool is_image = false;

	// Verbosity ftw.
	bool verbose = false;

	// Default to '/dev/dvd' by default
	// FIXME check to see if the filename exists, and if not, poll /dev/sr0 instead
	char* device_filename = "/dev/dvd";

	/** Begin GNU getopt_long **/
	// Specific variables to getopt_long
	int long_index = 0;
	int opt;
	// Send 'invalid argument' to stderr
	opterr = 1;

	// The display_* functions are just false by default, enabled by passing options
	int display_all = 0;
	int display_id = 0;
	int display_title = 0;
	int display_num_tracks = 0;
	int display_num_vts = 0;
	int display_provider_id = 0;
	int display_serial_id = 0;
	int display_vmg_id = 0;
	int display_side = 0;
	int display_video_format = 0;
	int display_video_codec = 0;
	int display_aspect_ratio = 0;
	int display_video_height = 0;
	int display_video_width = 0;
	int display_letterbox = 0;
	int display_film_mode = 0;
	int display_cc = 0;
	int display_num_audio_streams = 0;
	int display_num_subtitles;

	// Not enabled by an argument, set manually
	bool display_track = false;

	// Retain argument passed for track
	char *track_number_optarg;

	struct option long_options[] = {

		// Entries with both a name and a value, will take either the
		// long option or the short one.  Fex, '--device' or '-i'
		{ "device", required_argument, 0, 'i' },
		{ "track", required_argument, 0, 't' },

		{ "verbose", no_argument, 0, 'v' },

		// These set the value of the display_* variables above,
		// directly to 1 (true) if they are passed.  Only the long
		// option will trigger them.  fex, '--num-tracks'
		{ "all", no_argument, & display_all, 1 },

		// DVD
		{ "id", no_argument, & display_id, 1 },
		{ "title", no_argument, & display_title, 1 },
		{ "num-tracks", no_argument, & display_num_tracks, 1 },
		{ "num-vts", no_argument, & display_num_vts, 1 },
		{ "provider-id", no_argument, & display_provider_id, 1 },
		{ "serial-id", no_argument, & display_serial_id, 1 },
		{ "vmg-id", no_argument, & display_vmg_id, 1 },
		{ "side", no_argument, & display_side, 1 },

		// FIXME add a warning if track information is requested without
		// specifying a track number.
		// Video
		{ "video-format", no_argument, & display_video_format, 1 },
		{ "video-codec", no_argument, & display_video_codec, 1 },
		{ "aspect-ratio", no_argument, & display_aspect_ratio, 1 },
		{ "video-height", no_argument, & display_video_height, 1 },
		{ "video-width", no_argument, & display_video_width, 1 },
		{ "letterbox", no_argument, & display_letterbox, 1 },
		{ "film-mode", no_argument, & display_film_mode, 1 },

		// Audio
		{ "num-audio-streams", no_argument, & display_num_audio_streams, 1 },

		// Subtitles
		{ "num-subtitles", no_argument, & display_num_subtitles, 1 },
		{ "cc", no_argument, & display_cc, 1 },

		{ 0, 0, 0, 0 }
	};

	// I could probably come up with a better variable name. I probably would if
	// I understood getopt better. :T
	char *str_options;
	str_options = "hi:t:v";

	// Check for invalid input
	bool valid_args = true;

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
				if(atoi(optarg) < 1 || atoi(optarg) > 99) {
					fprintf(stderr, "Track number must be between 1 and 99\n");
					valid_args = false;
				} else {
					track_number = atoi(optarg);
					display_track = true;
				}
				break;

			case 'v':
				verbose = true;
				break;

			// ignore unknown arguments
			case '?':
				break;

			// let getopt_long set the variable
			case 0:
				break;

			default:
				break;
		}
	}

	// Exit after all invalid input warnings have been sent
	if(valid_args == false)
		return 1;

	// If '-i /dev/device' is not passed, then set it to the string
	// passed.  fex: 'dvd_info /dev/dvd1' would change it from the default
	// of '/dev/dvd'.
	if (argv[optind]) {
		device_filename = argv[optind];
	}

	// Verbose output begins
	if(verbose)
		printf("dvd: %s\n", device_filename);

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
		if(verbose) {
			printf("drive status: ");
			dvd_drive_display_status(device_filename);
		}
	}

	// Wait for the drive to become ready
	// FIXME, make this optional?  Dunno.  Probably not.
	// At the very least, let the wait value be set. :)
	if(is_hardware) {
		if(!dvd_drive_has_media(device_filename)) {

			// sleep for one second
			int sleepy_time = 1000000;
			// how many naps I have taken
			int num_naps = 0;
			// when to stop napping (60 seconds)
			int max_num_naps = 60;

			// FIXME send to stderr
			printf("drive status: ");
			dvd_drive_display_status(device_filename);

			fprintf(stderr, "dvd_info: waiting for media\n");
			fprintf(stderr, "dvd_info: will give up after one minute\n");
			while(!dvd_drive_has_media(device_filename) && (num_naps < max_num_naps)) {
				usleep(sleepy_time);
				num_naps = num_naps + 1;

				// This is slightly annoying, even for me.
				if(verbose)
					fprintf(stderr, "%i ", num_naps);

				// Tired of waiting, exiting out
				if(num_naps == max_num_naps) {
					if(verbose)
						fprintf(stderr, "\n");
					fprintf(stderr, "dvd_info: tired of waiting for media, quitting\n");
					return 1;
				}
			}
		}
	}

	// begin libdvdnav usage
	dvdnav_t *dvdnav_dvd;
	dvdnav_status_t *dvdnav_status;
	int dvdnav_ret;
	dvdnav_ret = dvdnav_open(&dvdnav_dvd, device_filename);

	if(dvdnav_ret == DVDNAV_STATUS_ERR) {
		printf("error opening %s with dvdnav\n", device_filename);
	}

	// begin libdvdread usage

	// Open DVD device
	dvd_reader_t *dvdread_dvd;
	dvdread_dvd = DVDOpen(device_filename);

	// Open IFO zero -- where all the cool stuff is
	ifo_handle_t *ifo_zero;
	ifo_zero = ifoOpen(dvdread_dvd, 0);
	// FIXME do a proper exit
	if(!ifo_zero) {
		fprintf(stderr, "dvd_info: opening IFO zero failed\n");
		return 1;
	}

	// --id
	// Display DVDDiscID from libdvdread
	if(display_id || display_all) {

		int dvd_disc_id;
		unsigned char tmp_buf[16];

		dvd_disc_id = DVDDiscID(dvdread_dvd, tmp_buf);

		if(dvd_disc_id == -1) {
			fprintf(stderr, "dvd_info: querying DVD id failed\n");
		} else {

			if(verbose)
				printf("disc_id: ");

			for(int x = 0; x < sizeof(tmp_buf); x++) {
				printf("%02x", tmp_buf[x]);
			}
			printf("\n");

		}

	}

	// --title
	// Display DVD title
	if(display_title || display_all) {

		/*
		const char *dvd_title;

		if(dvdnav_get_title_string(dvdnav_dvd, &dvd_title) == DVDNAV_STATUS_OK) {
			if(verbose)
				printf("title: ");
			printf("%s\n", dvd_title);
		}
		*/

		char title[33] = {'\0'};
		dvd_device_title(device_filename, title);
		if(verbose)
			printf("title: ");
		printf("%s\n", title);

	}

	// --num-tracks
	// Display total number of tracks
	if((display_num_tracks || display_all) && ifo_zero) {

		int num_tracks;

		num_tracks = dvd_info_num_tracks(ifo_zero);

		if(verbose)
			printf("tracks: ");
		printf("%i\n", num_tracks);

	}

	// --num-vts
	// Display number of VTSs on DVD
	if((display_num_vts || display_all) && ifo_zero) {

		int num_vts;

		num_vts = dvd_info_num_vts(ifo_zero);

		if(verbose)
			printf("num vts: ");
		printf("%i\n", num_vts);

	} else if((display_num_vts || display_all) && !ifo_zero) {

		fprintf(stderr, "dvd_info: cannot display num vts\n");

	}

	// --provider-id
	// Display provider ID
	if((display_provider_id || display_all) && ifo_zero) {

		// Max length of provider ID is 32 letters, so create an array
		// that has enough size to store the letters and a null
		// terminator.  Also initialize it with all null terminators.
		char provider_id[33] = {'\0'};
		dvd_info_provider_id(ifo_zero, provider_id);

		if(verbose)
			printf("provider id: ");
		printf("%s\n", provider_id);

		// Having an empty provider ID is very common.
		bool has_provider_id = false;
		if(provider_id[0] != '\0')
			has_provider_id = true;

	} else if((display_provider_id || display_all) && !ifo_zero) {

		fprintf(stderr, "dvd_info: cannot display provider id\n");

	}

	// --serial-id
	// Display serial ID
	if(display_serial_id || display_all) {

		char serial_id[17] = {'\0'};
		dvd_info_serial_id(dvdnav_dvd, serial_id);

		if(verbose)
			printf("serial id: ");
		printf("%s\n", serial_id);

	}

	// --vmg -id
	// Display VMG ID
	if((display_vmg_id || display_all) && ifo_zero) {

		char vmg_id[13] = {'\0'};
		dvd_info_vmg_id(ifo_zero, vmg_id);

		if(verbose)
			printf("vmg id: ");
		printf("%s\n", vmg_id);

	}

	// --side
	// Display disc side
	if(display_side || display_all) {
		if(verbose)
			printf("side: ");
		printf("%i\n", ifo_zero->vmgi_mat->disc_side);
	}

	/**
	 * Track information
	 */
	ifo_handle_t *track_ifo;
	char *video_codec;
	char *video_format;
	char *aspect_ratio;

	if(display_track) {

		track_ifo = ifoOpen(dvdread_dvd, track_number);
		// FIXME do a proper exit
		if(!track_ifo) {
			fprintf(stderr, "dvd_info: opening IFO %i failed\n", track_number);
			return 1;
		}

		/*
		char video_codec[6];
		dvd_track_video_codec(track_ifo, video_codec);
		*/

		// Video codec
		bool valid_video_codec = true;
		if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
			video_codec = "MPEG1";
		else if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
			video_codec = "MPEG2";
		else {
			video_codec = "Unknown";
			valid_video_codec = false;
		}

		// Video format and height
		unsigned int video_height;
		bool valid_video_format = true;
		bool valid_video_height = true;
		if(track_ifo->vtsi_mat->vts_video_attr.video_format == 0) {
			video_format = "NTSC";
			video_height = 480;
		} else if(track_ifo->vtsi_mat->vts_video_attr.video_format == 1) {
			video_format = "PAL";
			video_height = 576;
		} else {
			video_format = "Unknown";
			valid_video_format = false;
			valid_video_height = false;
		}

		// Aspect ratio
		bool valid_aspect_ratio = true;
		if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
			aspect_ratio = "4:3";
		else if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
			aspect_ratio = "16:9";
		else {
			aspect_ratio = "Unknown";
			valid_aspect_ratio = false;
		}

		// Video width
		unsigned int video_width = 720;
		bool valid_video_width = true;
		if(track_ifo->vtsi_mat->vts_video_attr.picture_size == 0) {
			video_width = 720;
		} else if(track_ifo->vtsi_mat->vts_video_attr.picture_size == 1) {
			video_width = 704;
		} else if(track_ifo->vtsi_mat->vts_video_attr.picture_size == 2) {
			video_width = 352;
		} else if(track_ifo->vtsi_mat->vts_video_attr.picture_size == 3) {
			video_width = 352;
			video_height = video_height / 2;
		}

		// Letterbox
		bool letterbox = false;
		if(track_ifo->vtsi_mat->vts_video_attr.letterboxed)
			letterbox = true;

		// Film mode: film (movie), video (camera)
		char *film_mode;
		if(track_ifo->vtsi_mat->vts_video_attr.film_mode)
			film_mode = "film";
		else
			film_mode = "video";

		// Closed Captioning
		bool has_cc = false;
		bool has_cc_1 = false;
		bool has_cc_2 = false;
		if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_1 || track_ifo->vtsi_mat->vts_video_attr.line21_cc_2) {
			has_cc = true;
			if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_1)
				has_cc_1 = true;
			if(track_ifo->vtsi_mat->vts_video_attr.line21_cc_2)
				has_cc_2 = true;
		}

		// Audio streams
		int num_audio_streams;
		num_audio_streams = track_ifo->vtsi_mat->nr_of_vts_audio_streams;

		// Subtitles
		int num_subtitles;
		num_subtitles = track_ifo->vtsi_mat->nr_of_vts_subp_streams;

		// --video-codec
		// Display video codec
		if(display_video_codec || display_all) {
			if(verbose)
				printf("video codec: ");
			printf("%s\n", video_codec);
		}

		// --video-format
		// Display video format
		if(display_video_format || display_all) {
			if(verbose)
				printf("video format: ");
			printf("%s\n", video_format);
		}

		// --aspect-ratio
		// Display aspect ratio
		if(display_aspect_ratio || display_all) {
			if(verbose)
				printf("aspect ratio: ");
			printf("%s\n", aspect_ratio);
		}

		// --video-height
		// Display video height
		if(display_video_height || display_all) {
			if(verbose)
				printf("video height: ");
			printf("%i\n", video_height);
		}

		// --video-width
		// Display video width
		if(display_video_width || display_all) {
			if(verbose)
				printf("video width: ");
			printf("%i\n", video_width);
		}

		// --letterbox
		// Display letterbox
		if(display_letterbox || display_all) {
			if(verbose)
				printf("letterbox: ");
			if(letterbox)
				printf("1\n");
			else
				printf("0\n");
		}

		// --film-mode
		// Film mode
		if(display_film_mode || display_all) {
			if(verbose)
				printf("film mode: ");
			printf("%s\n", film_mode);
		}

		// Display Closed Captioning
		// Not sure if this is right or not
		/*
		if(display_cc || display_all) {
			if(verbose)
				printf("closed captioning: ");
			printf("1\n");
		}
		*/

		// --num-audio-streams
		// Display number of audio streams
		if(display_num_audio_streams || display_all) {
			if(verbose)
				printf("audio streams: ");
			printf("%i\n", num_audio_streams);
		}

		// --num-subtitles
		// Display number of subtitles
		if(display_num_subtitles || display_all) {
			if(verbose)
				printf("subtitles: ");
			printf("%i\n", num_subtitles);
		}

	}

	// Cleanup

	if(ifo_zero)
		ifoClose(ifo_zero);

	if(display_track && track_ifo)
		ifoClose(track_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	if(dvdnav_dvd)
		dvdnav_close(dvdnav_dvd);
}
