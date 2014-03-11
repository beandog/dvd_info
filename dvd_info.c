#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <linux/cdrom.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/dvd_udf.h>
#include <dvdread/ifo_read.h>
#include <dvdnav/dvdnav.h>
#include "dvd_device.h"
#include "dvd_drive.h"

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
	printf("  --num_tracks		Number of tracks\n");
	printf("  --num_vts		Number of VTSs\n");
	printf("  --provider_id 	Provider ID\n");
	printf("  --vmg_id		VMG ID\n");

}

/**
 * Get the DVD title, which maxes out at a 32-character string.
 * All DVDs should have one.
 *
 * This whole function is mostly lifted from lsdvd.
 *
 */
int dvd_info_title(char* device_filename, char *dvd_title) {

	char title[33];
	FILE* filehandle = 0;
	int x, y, z;

	// If we can't even open the device, exit quietly
	filehandle = fopen(device_filename, "r");
	if(filehandle == NULL) {
		return 1;
	}

	// The DVD title is actually on the disc, and doesn't need the dvdread
	// or dvdnav library to access it.  I should prefer to use them, though
	// to avoid situations where something freaks out for not decrypting
	// the CSS first ... so, I guess a FIXME is in order.
	if(fseek(filehandle, 32808, SEEK_SET) == -1) {
		fclose(filehandle);
		return 2;
	}

	x = fread(title, 1, 32, filehandle);
	if(x == 0) {
		fclose(filehandle);
		return 3;
	}
	title[32] = '\0';

	fclose(filehandle);

	// A nice way to trim the string. :)
	y = sizeof(title);
	while(y-- > 2) {
		if(title[y] == ' ') {
			title[y] = '\0';
		}
	}

	// For future note to myself, this answers the question of
	// how to have a function 'return' a string, and still be able to have
	// exit codes as well.  The answer is that you don't return a string at
	// all -- you copy the contents to the string that is passed in the
	// function.
	strncpy(dvd_title, title, 32);

	return 0;

}

int main(int argc, char **argv) {

	// DVD file descriptor
	int dvd_fd;

	// DVD drive status
	int drive_status;

	// DVD track number -- default to 0, which basically means, ignore me.
	int track_number = 0;

	// Do a check to see if the DVD filename given is hardware ('/dev/foo')
	bool is_hardware;
	// Or if it's an image file, filename (ISO, UDF, etc.)
	bool is_image;

	// Verbosity ftw.
	bool verbose = false;

	// Default to '/dev/dvd' by default
	// FIXME check to see if the filename exists, and if not, poll /dev/sr0 instead
	char* device_filename = "/dev/dvd";

	/** Begin GNU getopt_long **/
	// Specific variables to getopt_long
	int long_index = 0;
	int opt;
	// Suppress getopt sending 'invalid argument' to stderr
	// FIXME: can add this once I get proper error handling myself
	// opterr = 0;

	// The display_* functions are just false by default, enabled by passing options
	int display_all = 0;
	int display_id = 0;
	int display_title = 0;
	int display_num_tracks = 0;
	int display_num_vts = 0;
	int display_provider_id = 0;
	int display_serial_id = 0;
	int display_vmg_id = 0;

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
		{ "id", no_argument, & display_id, 1 },
		{ "title", no_argument, & display_title, 1 },
		{ "num-tracks", no_argument, & display_num_tracks, 1 },
		{ "num-vts", no_argument, & display_num_vts, 1 },
		{ "provider-id", no_argument, & display_provider_id, 1 },
		{ "serial-id", no_argument, & display_serial_id, 1 },
		{ "vmg-id", no_argument, & display_vmg_id, 1 },

		{ 0, 0, 0, 0 }
	};

	// I could probably come up with a better variable name. I probably would if
	// I understood getopt better. :T
	char *str_options;
	str_options = "hi:t:v";

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

	// If '-i /dev/device' is not passed, then set it to the string
	// passed.  fex: 'dvd_info /dev/dvd1' would change it from the default
	// of '/dev/dvd'.
	if (argv[optind]) {
		device_filename = argv[optind];
	}

	// Verbose output begins
	if(verbose)
		printf("dvd: %s\n", device_filename);

	// Reset track number if needed
	if(track_number < 1)
		track_number = 0;

	/** Begin dvd_info :) */

	// Check to see if device can be accessed
	if(dvd_device_access(device_filename) == 1) {
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

	// open DVD device and don't cache queries (FIXME? do I need to set that?)
	dvd_reader_t *dvdread_dvd;
	dvdread_dvd = DVDOpen(device_filename);
	// DVDUDFCacheLevel(dvd, 0);

	// Open IFO zero -- where all the cool stuff is
	ifo_handle_t *ifo_zero;
	ifo_zero = ifoOpen(dvdread_dvd, 0);
	if(!ifo_zero) {
		fprintf(stderr, "dvd_info: opening IFO zero failed\n");
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
				printf("id: ");

			for(int x = 0; x < sizeof(tmp_buf); x++) {
				printf("%02x", tmp_buf[x]);
			}
			printf("\n");

		}

	}

	// --num_tracks
	if((display_num_tracks || display_all) && ifo_zero) {

		int num_tracks;

		num_tracks = ifo_zero->tt_srpt->nr_of_srpts;

		if(verbose)
			printf("num_tracks: ");
		printf("%i\n", num_tracks);

	}

	// --num_vts
	// Display number of VTSs on DVD
	if((display_num_vts || display_all) && ifo_zero) {

		int num_vts;

		num_vts = ifo_zero->vts_atrt->nr_of_vtss;

		if(verbose)
			printf("num_vts: ");
		printf("%i\n", num_vts);

	} else if((display_num_vts || display_all) && !ifo_zero) {

		fprintf(stderr, "dvd_info: cannot display num_vts\n");

	}

	// --provider_id
	// Display provider ID
	if((display_provider_id || display_all) && ifo_zero) {

		char *provider_id;
		bool has_provider_id = false;

		provider_id = ifo_zero->vmgi_mat->provider_identifier;

		if(verbose)
			printf("provider_id: ");
		printf("%s\n", provider_id);

		// Having an empty provider ID is very common.
		if(provider_id[0] != '\0')
			has_provider_id = true;

	} else if((display_provider_id || display_all) && !ifo_zero) {

		fprintf(stderr, "dvd_info: cannot display provider_id\n");

	}

	// --serial-id
	// Display serial ID
	if(display_serial_id || display_all) {

		const char *serial_id;

		if(dvdnav_get_serial_string(dvdnav_dvd, &serial_id) == DVDNAV_STATUS_OK) {

			if(verbose)
				printf("serial_id: ");
			printf("%s\n", serial_id);

		}

	}

	// --title
	// Display DVD title
	if(display_title || display_all) {

		char dvd_title[33];
		int dvd_info_title_ret;

		dvd_info_title_ret = dvd_info_title(device_filename, dvd_title);

		if(dvd_info_title_ret == 1) {
			fprintf(stderr, "dvd_info: could not open device %s for reading\n", device_filename);
		} else if(dvd_info_title_ret == 2) {
			fprintf(stderr, "dvd_info: could not seek on device %s\n", device_filename);
		} else if(dvd_info_title_ret == 3) {
			fprintf(stderr, "dvd_info: could not read device %s\n", device_filename);
		} else {
			if(verbose)
				printf("title: ");
			printf("%s\n", dvd_title);
		}

	}

	/**
	 * Display VMG_ID
	 *
	 * It's entirely possible, and common, that the string is blank.  If it's not
	 * blank, it is probably 'DVDVIDEO-VMG'.
	 *
	 */
	if((display_vmg_id || display_all) && ifo_zero) {

		char *vmg_id;

		vmg_id = ifo_zero->vmgi_mat->vmg_identifier;
		vmg_id[12] = '\0';

		if(verbose)
			printf("vmg_id: ");
		printf("%s\n", vmg_id);

	}

	// Cleanup

	if(ifo_zero)
		ifoClose(ifo_zero);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	if(dvdnav_dvd)
		dvdnav_close(dvdnav_dvd);
}
