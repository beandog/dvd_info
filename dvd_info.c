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
#include "dvd_device.h"
#include "dvd_drive.h"

int main(int argc, char **argv) {

	int dvd_fd;
	int drive_status;
	int track_number = 0;
	bool is_hardware;
	bool is_image;
	bool ready = false;
	bool verbose = false;
	char* device_filename;
	char* status;

	// getopt_long
	int display_id = 0;
	int display_num_vts = 0;
	int long_index = 0;
	int opt;
	// Suppress getopt sending 'invalid argument' to stderr
	// opterr = 0;
	struct option long_options[] = {
		{ "device", required_argument, 0, 'i' },
		{ "track", required_argument, 0, 't' },
		{ "verbose", no_argument, 0, 'v' },
		{ "num_vts", no_argument, & display_num_vts, 1 },
		{ "id", no_argument, & display_id, 1 },
		{ 0, 0, 0, 0 }
	};

	device_filename = "/dev/dvd";

	while((opt = getopt_long(argc, argv, ":i:t:v", long_options, &long_index )) != -1) {

		switch(opt) {

			case 'i':
				device_filename = optarg;
				break;

			case 't':
				track_number = 0;
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

	// Use non-argument string as DVD device, if there is one
	if(strlen(argv[optind]) > 0) {
	 	device_filename = argv[optind];
	}

	if(verbose)
		printf("device filename: %s\n", device_filename);

	// Handle options
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

	if(is_image)
		ready = true;

	// Poll drive status if it is hardware
	if(is_hardware)
		drive_status = dvd_drive_get_status(device_filename);

	if(is_hardware)
		dvd_drive_display_status(device_filename);

	// Wait for the drive to become ready
	if(is_hardware) {
		if(dvd_drive_has_media(device_filename)) {
			ready = true;
		} else  {
			printf("waiting for drive to become ready\n");
			while(!dvd_drive_has_media(device_filename)) {
				usleep(1000000);
			}
		}
		ready = true;
	} else {
		ready = true;
	}

	// libdvdread

	// open DVD device and don't cache queries
	dvd_reader_t *dvd;
	dvd = DVDOpen(device_filename);
	DVDUDFCacheLevel(dvd, 0);

	ifo_handle_t *ifo_zero;
	ifo_zero = ifoOpen(dvd, 0);
	if(!ifo_zero) { fprintf(stderr, "opening ifo_zero failed\n"); return 1; }

	int nr_of_vtss = ifo_zero->vts_atrt->nr_of_vtss;
	printf("nr_of_vtss: %i\n", nr_of_vtss);

	ifoClose(ifo_zero);
	DVDClose(dvd);

}
