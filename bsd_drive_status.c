#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#if defined (__FreeBSD__)
#define DEFAULT_DVD_RAW_DEVICE "/dev/cd0"
#elif defined (__NetBSD__)
#include <util.h>
#define DEFAULT_DVD_RAW_DEVICE "/dev/rcd0d"
#elif defined (__OpenBSD__)
#include <util.h>
#define DEFAULT_DVD_RAW_DEVICE "/dev/rcd0c"
#endif
#include "dvd_device.h"

/**
 * bsd_drive_status.c
 * Get the status of a DVD drive tray
 *
 * This program does its best to tell you if there is a DVD tray in the drive,
 * not the SCSI status of the device (doing so would require lower-level code).
 * Because of that, you may see some errors thrown to syslog while trying to
 * access filesystems that aren't there or devices that aren't ready.
 *
 * Exit codes:
 * 0 - ran succesfully
 * 1 - ran with errors
 * 2 - drive is closed with no disc OR drive is open
 * 3 - drive is closed with a DVD
 * 4 - drive is closed with a disc that is not a DVD
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

	const char *device_filename = NULL;
	const char *raw_device_filename = NULL;

	if(argc > 2) {
		device_filename = argv[1];
		raw_device_filename = argv[2];
	} else {
		device_filename = DEFAULT_DVD_DEVICE;
		raw_device_filename = DEFAULT_DVD_RAW_DEVICE;
	}

	int fd = -1;

	fd = open(raw_device_filename, O_RDONLY);

	if(fd < 0) {
		if(errno == EACCES)
			fprintf(stderr, "could not open device %s permission denied\n", raw_device_filename);
		else
			fprintf(stderr, "could not open device %s (errno: %i)\n", raw_device_filename, errno);
		return 1;
	}
	
	close(fd);

	fd = open(device_filename, O_RDONLY);

	if(fd < 0) {
#ifdef __NetBSD__
		if(errno == ENODEV)
#else
		if(errno == EIO)
#endif
		{
			printf("drive is closed with no disc or drive is open\n");
			return 2;
		} else {
			fprintf(stderr, "could not detect drive status (errno: %i)\n", errno);
			return 1;
		}

	}

	/**
	 * Look to see if the disc in the drive is a DVD or not.
	 * libdvdread will open any disc just fine, so scan it to see if there
	 * is a VMG IFO as well as an additional check.
	 */
	dvd_reader_t *dvdread_dvd = NULL;
	
	// FreeBSD and NetBSD will throw syslog messages if there's a disc in the drive that's
	// not a DVD.
	dvdread_dvd = DVDOpen(device_filename);
	if(dvdread_dvd) {

		ifo_handle_t *vmg_ifo = NULL;
		
		// OpenBSD will throw a syslog error here if the tray is open
		vmg_ifo = ifoOpen(dvdread_dvd, 0);

		if(vmg_ifo == NULL) {
			ifoClose(vmg_ifo);
			DVDClose(dvdread_dvd);
			close(fd);
#ifdef __OpenBSD__
			printf("drive is closed with no disc or non-DVD or drive is open\n");
			return 2;
#else
			printf("drive is closed with a disc that is not a DVD\n");
			return 4;
#endif
		} else {
			printf("drive is closed with a DVD\n");
			DVDClose(dvdread_dvd);
			close(fd);
			return 3;
		}

	}

	close(fd);

	return 0;

}
