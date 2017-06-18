#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dvdread/dvd_reader.h>
#if defined (__NetBSD__) || defined (__OpenBSD__)
#include <util.h>
#endif
#ifdef __FreeBSD__
#define DEFAULT_DVD_RAW_DEVICE "/dev/cd0"
#endif
#ifdef __NetBSD__
#define DEFAULT_DVD_RAW_DEVICE "/dev/rcd0d"
#endif
#ifdef __OpenBSD__
#define DEFAULT_DVD_RAW_DEVICE "/dev/rcd0c"
#endif
#include "dvd_device.h"

/**
 * bsd_drive_status.c
 * Get the status of a DVD drive tray
 */

/*
 *   ___                   ____ ____  ____  
 *  / _ \ _ __   ___ _ __ | __ ) ___||  _ \
 * | | | | '_ \ / _ \ '_ \|  _ \___ \| | | |
 * | |_| | |_) |  __/ | | | |_) |__) | |_| |
 *  \___/| .__/ \___|_| |_|____/____/|____/ 
 *       |_|                                
 *  
 * To compile:
 * gcc -o dvd_drive_status bsd_drive_status.c -I/usr/local/include -L/usr/local/lib -ldvdread -lutil
 *
 * Exit codes:
 * 0 - ran succesfully
 * 1 - ran with errors
 * 2 - drive is closed with no disc OR drive is open
 * 3 - drive is closed with a disc
 *
 * OpenBSD's kernel doesn't seem to have a 'polling' status for the drive,
 * because it doesn't allow opening the raw device while it's in that state.
 *
 * The raw block device (/dev/rcd*c) has to be opened before the block device does (/dev/cd*c) or
 * the kernel will throw a warning:
 * cd0(atapiscsi0:0:0): Check Condition (error 0x70) on opcode 0x0
 *   SENSE KEY: Not Ready
 *    ASC/ASCQ: Medium Not Present
 *
 *  _   _      _   ____ ____  ____  
 * | \ | | ___| |_| __ ) ___||  _ \
 * |  \| |/ _ \ __|  _ \___ \| | | |
 * | |\  |  __/ |_| |_) |__) | |_| |
 * |_| \_|\___|\__|____/____/|____/ 
 *                                
 * 
 * To compile:
 * gcc -o dvd_drive_status bsd_drive_status.c -I/usr/pkg/include -Wl,-R/usr/pkg/lib -L/usr/pkg/lib -ldvdread -lutil
 *
 *  _____              ____ ____  ____  
 * |  ___| __ ___  ___| __ ) ___||  _ \
 * | |_ | '__/ _ \/ _ \  _ \___ \| | | |
 * |  _|| | |  __/  __/ |_) |__) | |_| |
 * |_|  |_|  \___|\___|____/____/|____/ 
 *
 *
 * To compile:
 * gcc -o dvd_drive_status bsd_drive_status.c -I/usr/local/include -L/usr/local/lib -ldvdread
 *
 */

int main(int argc, char **argv) {

	if(argc == 2 || argc > 3 || (argc > 1 && strncmp(&argv[0][0], "-", 1) == 0)) {
		printf("dvd_drive_status [dvd_device dvd_raw_device]\n");
		printf("Default devices: %s %s\n", DEFAULT_DVD_DEVICE, DEFAULT_DVD_RAW_DEVICE);
		return 1;
	}

	char *device_filename;
	char *raw_device_filename;

	if(argc > 2) {
		device_filename = argv[1];
		raw_device_filename = argv[2];
	} else {
		device_filename = DEFAULT_DVD_DEVICE;
		raw_device_filename = DEFAULT_DVD_RAW_DEVICE;
	}

	int fd;

	fd = open(raw_device_filename, O_RDONLY);

	if(fd < 0) {
		fprintf(stderr, "Could not open device %s\n", raw_device_filename);
		return 1;
	}
	
	close(fd);

	fd = open(DEFAULT_DVD_DEVICE, O_RDONLY);

	if(fd < 0) {
		if(errno == EIO) {
			printf("drive is closed with no disc OR drive is open\n");
			close(fd);
			return 2;
		// I've somehow thrown this error a few times, but don't know how :| 
		/*
		} else if(errno == ENXIO || errno == ENOMEDIUM) {
			printf("drive closed with no disc\n");
			close(fd_raw_device);
			return 2;
		*/
		} else {
			close(fd);
			fprintf(stderr, "something unexpected happnd ... send a bug report! returned errno: %i\n", errno);
			return 1;
		}

	}

	if(DVDOpen(device_filename)) {

		printf("drive closed with disc\n");
		close(fd);
		return 3;

	}

	close(fd);

	return 0;

}
