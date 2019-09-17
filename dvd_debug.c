#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include "dvd_audio.h"
#include "dvd_chapter.h"
#include "dvd_device.h"
#include "dvd_info.h"
#include "dvd_subtitles.h"
#include "dvd_time.h"
#include "dvd_track.h"
#include "dvd_video.h"
#include "dvd_vmg_ifo.h"
#include "dvd_vob.h"
#include "dvd_vts.h"
#include <dvdread/ifo_print.h>
#ifdef __linux__
#include <linux/cdrom.h>
#include "dvd_drive.h"
#endif
#ifndef DVD_INFO_VERSION
#define DVD_INFO_VERSION "1.7_beta1"
#endif

int main(int argc, char **argv) {

	const char *device_filename = NULL;
	uint16_t track = 1;
	if (argc == 2)
		device_filename = argv[1];
	else if(argc == 3) {
		device_filename = argv[1];
		track = (uint16_t)strtoumax(argv[2], NULL, 10);
	} else
		device_filename = "/dev/sr0";

	/** Begin dvd_debug :) */

	// Check to see if device can be accessed
	if(!dvd_device_access(device_filename)) {
		fprintf(stderr, "Cannot access %s\n", device_filename);
		return 1;
	}

	// Check to see if device can be opened
	int fd = 0;
	fd = dvd_device_open(device_filename);
	if(fd < 0) {
		fprintf(stderr, "Could not open %s\n", device_filename);
		return 1;
	}
	dvd_device_close(fd);

#ifdef __linux__

	// Poll drive status if it is hardware
	if(dvd_device_is_hardware(device_filename)) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "DVD drive status: ");
			dvd_drive_display_status(device_filename);

			return 1;

		}

	}

#endif

	// begin libdvdread usage

	// Open DVD device
	dvd_reader_t *dvdread_dvd = NULL;
	dvdread_dvd = DVDOpen(device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "Opening DVD %s failed\n", device_filename);
		return 1;
	}

	// Open VMG IFO -- where all the cool stuff is
	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "Opening VMG IFO failed\n");
		return 1;
	}

	uint16_t vts = 1;

	if(argc != 3) {
		vts = 0;
		printf("[VMG IFO]\n");
	} else {
		vts = dvd_vts_ifo_number(vmg_ifo, track);
		printf("[VTS %" PRIu16 ", Track %" PRIu16 "]\n", vts, track);
	}
	ifo_print(dvdread_dvd, vts);

	/*
	ifo_handle_t *vts_ifo = NULL;
	for(vts = 1; vts < dvd_video_title_sets(vmg_ifo) + 1; vts++) {

		vts_ifo = ifoOpen(dvdread_dvd, vts);

		if(vts_ifo == NULL) {
			printf("[VTS %" PRIu16 "]\n", vts);
			printf("* Opening IFO failed, skipping\n");
			continue;
		}

		track = 1;
		while(track < vmg_ifo->tt_srpt->nr_of_srpts + 1) {

			if(dvd_vts_ifo_number(vmg_ifo, track) == vts) {

				printf("[VTS %" PRIu16 ", Track %" PRIu16 "]\n", vts, track);

				ifo_print(dvdread_dvd, track);


			}

			track++;

		}

	}
	*/

	return 0;

}
