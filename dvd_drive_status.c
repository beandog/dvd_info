#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>

/**
 * From linux/cdrom.h:
 * CDS_NO_DISC             1
 * CDS_TRAY_OPEN           2
 * CDS_DRIVE_NOT_READY     3
 * CDS_DISC_OK             4
 */

/**
 * dvd_drive_status.c
 * Get the status of the disc tray
 *
 * See http://dvds.beandog.org/doku.php/dvd_drive_status for justification :)
 *
 * This does do strict error checking to see if the device exists, is a DVD
 * drive, is accessible, and so on.
 *
 * With no argument, uses '/dev/dvd' as the drive
 *
 * Exit codes:
 * 1 - no disc (closed, no media)
 * 2 - tray open
 * 3 - drive not ready (opening or polling)
 * 4 - drive ready (closed, has media)
 * 5 - device exists, but is NOT a DVD drive
 * 6 - cannot access device
 *
 * - Returns an exit code similar to CDROM_DRIVE_STATUS in cdrom.h
 */
int main(int argc, char **argv) {

	int cdrom;
	int drive_status;
	char* device_filename;
	char* status;

	// Use '/dev/dvd' by default
	if(argc == 1)
		device_filename = "/dev/dvd";
	else
		device_filename = argv[1];

	// Check if device exists
	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "cannot access %s\n", device_filename);
		return 6;
	}

	// Try opening device
	cdrom = open(device_filename, O_RDONLY | O_NONBLOCK);
	if(cdrom < 0) {
		fprintf(stderr, "error opening %s\n", device_filename);
		return 6;
	}

	// Fetch status
	drive_status = ioctl(cdrom, CDROM_DRIVE_STATUS);
	if(drive_status < 0) {
		fprintf(stderr, "%s is not a DVD drive\n", device_filename);
		close(cdrom);
		return 5;
	}

	close(cdrom);

	switch(drive_status) {
		case CDS_NO_DISC:
			status = "no disc";
			break;
		case CDS_TRAY_OPEN:
			status = "tray open";
			break;
		case CDS_DRIVE_NOT_READY:
			status = "drive not ready";
			break;
		case CDS_DISC_OK:
			status = "drive ready";
			break;
		default:
			status = "unknown";
			break;
	}

	printf("%s\n", status);

	return drive_status;
}
