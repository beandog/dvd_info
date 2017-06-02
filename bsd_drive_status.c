#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <util.h>
#include <dvdread/dvd_reader.h>

#define DVD_INFO_DEFAULT_DVD_RAW_DEVICE "/dev/rcd0c"

/**
 * bsd_drive_status.c
 *
 * Get the status of a DVD drive tray (OpenBSD port)
 *  
 * Exit codes:
 * 0 - ran succesfully
 * 1 - ran with errors
 * 2 - drive is closed with no disc OR drive is open
 * 3 - drive is closed with a disc
 *
 * To compile:
 * gcc -o dvd_drive_status bsd_drive_status.c -lutil -ldvdread -L/usr/local/lib -I/usr/local/include
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
 */

int main(int argc, char **argv) {

	// Device filename length will never exceed 11 chars, and check for any "-" flags passed
	if((argc > 1 && strlen(argv[1]) > 11) || (argc > 2) || (argc > 1 && (strncmp(argv[1], "-", 1) == 0))) {
		printf("dvd_drive_status [device]\n");
		return 0;
	}

	char *device_filename;
	char raw_device[11] = {'\0'};
	char block_device[10] = {'\0'};
	bool valid_device = false;

	if(argc > 1)
		device_filename = argv[1];
	else
		device_filename = DVD_INFO_DEFAULT_DVD_RAW_DEVICE;

	if(strlen(device_filename) == 3 && (strncmp(device_filename, "cd", 2) == 0) && isdigit(device_filename[2])) {
		valid_device = true;
		raw_device[8] = device_filename[2];
		snprintf(raw_device, 11, "/dev/rcd%cc", device_filename[2]);
		snprintf(block_device, 10, "/dev/cd%cc", device_filename[2]);
	}
	
	if(strlen(device_filename) == 9 && (strncmp(device_filename, "/dev/cd", 7) == 0) && isdigit(device_filename[7]) && device_filename[8] == 'c') {
		valid_device = true;
		snprintf(raw_device, 11, "/dev/rcd%cc", device_filename[7]);
		snprintf(block_device, 10, "/dev/cd%cc", device_filename[7]);
	}

	if(strlen(device_filename) == 10 && (strncmp(device_filename, "/dev/rcd", 8) == 0) && isdigit(device_filename[8]) && device_filename[9] == 'c') {
		valid_device = true;
		snprintf(raw_device, 11, "/dev/rcd%cc", device_filename[8]);
		snprintf(block_device, 10, "/dev/cd%cc", device_filename[8]);
	}

	if(!valid_device) {
		printf("invalid device: %s\n", device_filename);
		return 1;
	}

	int fd_raw_device;

	fd_raw_device = open(raw_device, O_RDONLY);

	if(fd_raw_device < 0) {
		printf("could not open device %s\n", raw_device);
		return 1;
	}

	int fd_block_device;

	fd_block_device = open(block_device, O_RDONLY);

	if(fd_block_device < 0) {
		if(errno == EIO) {
			printf("drive is closed with no disc OR drive is open\n");
			close(fd_raw_device);
			return 2;
		// I've somehow thrown this error a few times, but don't know how :| 
		} else if(errno == ENXIO || errno == ENOMEDIUM) {
			printf("drive closed with no disc\n");
			close(fd_raw_device);
			return 2;
		} else {
			close(fd_raw_device);
			printf("something unexpected happnd ... send a bug report! returned errno: %i\n", errno);
			return 1;
		}

	}

	if(DVDOpen(block_device)) {

		printf("drive closed with disc\n");
		close(fd_raw_device);
		close(fd_block_device);
		return 3;

	}

	close(fd_raw_device);
	close(fd_block_device);

	return 0;

}
