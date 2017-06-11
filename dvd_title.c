#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dvdread/dvd_reader.h>
#include "dvd_device.h"
#include "dvd_drive.h"
#include "dvd_vmg_ifo.h"
#ifndef VERSION
#define VERSION "1.0"
#endif

int main(int argc, char **argv) {

	const char program_name[] = "dvd_id";

	// Device hardware
	const char *device_filename = NULL;

	if (argc > 1 && argv[optind])
		device_filename = argv[optind];
	else
		device_filename = DEFAULT_DVD_DEVICE;

	// Check to see if device can be accessed
	if(!dvd_device_access(device_filename)) {
		fprintf(stderr, "%s: cannot access %s\n", program_name, device_filename);
		return 1;
	}

	// Check to see if device can be opened
	int dvd_fd = 0;

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
	dvd_reader_t *dvdread_dvd = NULL;

	dvdread_dvd = DVDOpen(device_filename);
	if(dvdread_dvd == NULL) {
		fprintf(stderr, "%s: Opening DVD %s failed\n", program_name, device_filename);
		return 1;
	}

	printf("%s\n", dvd_title(device_filename));

	DVDClose(dvdread_dvd);

	return 0;

}
