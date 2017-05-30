#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <util.h>
#include <dvdread/dvd_reader.h>

#define DVD_INFO_DEFAULT_DVD_DEVICE "cd0"
#define DVD_INFO_DEFAULT_DVD_RAW_DEVICE "/dev/rcd0c"
#define DVD_INFO_DEFAULT_DVD_BLOCK_DEVICE "/dev/cd0c"

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
 */

int main(int argc, char **argv) {

	// Device filename length will never exceed 11 chars
	if((argc > 1 && strlen(argv[1]) > 11) || (argc > 2) || (argc > 1 && (strncmp(argv[1], "-", 1) == 0))) {
		printf("dvd_drive_status [device]\n");
		return 0;
	}

	char *raw_device;
	char *block_device;
	int fd_raw_device = 0;
	int fd_block_device = 0;

	// In each case, switch to using the raw block device, since that is what
	// will be opened first
	if(argc == 1) {
		raw_device = DVD_INFO_DEFAULT_DVD_RAW_DEVICE;
		block_device = DVD_INFO_DEFAULT_DVD_BLOCK_DEVICE;
	} else if(argc == 2 && strncmp(argv[1], DVD_INFO_DEFAULT_DVD_DEVICE, strlen(DVD_INFO_DEFAULT_DVD_DEVICE)) == 0) {
		raw_device = DVD_INFO_DEFAULT_DVD_RAW_DEVICE;
		block_device = DVD_INFO_DEFAULT_DVD_BLOCK_DEVICE;
	} else if(argc == 2 && strncmp(argv[1], DVD_INFO_DEFAULT_DVD_BLOCK_DEVICE, strlen(DVD_INFO_DEFAULT_DVD_BLOCK_DEVICE)) == 0) {
		raw_device = DVD_INFO_DEFAULT_DVD_RAW_DEVICE;
		block_device = DVD_INFO_DEFAULT_DVD_BLOCK_DEVICE;
	} else {
		raw_device = argv[1];
		block_device = argv[1];
	}

	// Open the raw block device before the filesystem, to avoid
	// throwing a SCSI error to the syslog
	fd_raw_device = open(raw_device, O_RDONLY);
	if(fd_raw_device < 0) {
		printf("could not open raw device %s\n", raw_device);
		return 1;
	}

	fd_block_device = open(DVD_INFO_DEFAULT_DVD_BLOCK_DEVICE, O_RDONLY);
	if(fd_block_device < 0) {

		if(fd_block_device == -1 && errno == 5) {
			printf("drive is closed with no disc OR drive is open\n");
			close(fd_raw_device);
			return 2;
		}

		return 1;

	} else if(fd_block_device && errno == 0) {

		if(DVDOpen(block_device)) {

			printf("drive closed with disc\n");
			close(fd_raw_device);
			close(fd_block_device);
			return 3;

		}

	}

	close(fd_raw_device);
	close(fd_block_device);

	return 0;

}
