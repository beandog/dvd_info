#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>

#define DEFAULT_DVD_DEVICE "/dev/dvd"
#define PRIMARY_FALLBACK_DEVICE "/dev/sr0"
#define SECONDARY_FALLBACK_DEVICE "/dev/cdrom"

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
 * 7 - cannot find a device
 *
 * - Returns an exit code similar to CDROM_DRIVE_STATUS in cdrom.h
 */

bool device_access(const char *device_filename) {

	int a;
	bool success;

	a = access(device_filename, F_OK);

	if(a != 0)
		success = false;
	else
		success = true;

	return success;

}

int main(int argc, char **argv) {

	int cdrom;
	int drive_status;
	char* device_filename;
	char default_dvd_device[] = DEFAULT_DVD_DEVICE;
	char primary_fallback_device[] = PRIMARY_FALLBACK_DEVICE;
	char secondary_fallback_device[] = SECONDARY_FALLBACK_DEVICE;
	bool using_fallback_device = false;
	char* status;

	// Check if device exists
	if(argc == 1) {
		if(device_access(default_dvd_device)) {
			device_filename = default_dvd_device;
		} else if(device_access(primary_fallback_device)) {
			device_filename = primary_fallback_device;
			using_fallback_device = true;
		} else if(device_access(secondary_fallback_device)) {
			device_filename = secondary_fallback_device;
			using_fallback_device = true;
		} else {
			fprintf(stderr, "Could not guess your DVD device\n");
			fprintf(stderr, "Attempted opening %s, %s and %s with no luck\n", DEFAULT_DVD_DEVICE, PRIMARY_FALLBACK_DEVICE, SECONDARY_FALLBACK_DEVICE);
			return 7;
		}
		if(using_fallback_device) {
			fprintf(stderr, "Device not specified, using %s\n", device_filename);
		}
	} else {
		device_filename = argv[1];
		if(!device_access(device_filename)) {
			fprintf(stderr, "Cannot access %s\n", device_filename);
			return 6;
		}
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
			status = "drive closed with no disc";
			break;
		case CDS_TRAY_OPEN:
			status = "drive open";
			break;
		case CDS_DRIVE_NOT_READY:
			status = "drive not ready";
			break;
		case CDS_DISC_OK:
			status = "drive closed with disc";
			break;
		default:
			status = "unknown";
			break;
	}

	printf("%s\n", status);

	return drive_status;
}
