#include "dvd_drive.h"

#ifdef __linux__

/**
 * @file dvd_drive.h
 *
 * Functions to get status of DVD drive hardware tray
 *
 * There's two types of functions: ones that can be used externally to see if
 * there is a DVD or not, and others internally to get a closer look at the
 * actual status of the drive.
 *
 * To simply check if everything is READY TO GO, use dvd_drive_has_media()
 *
 * For example:
 *
 * if(dvd_drive_has_media("/dev/dvd"))
 * 	printf("Okay to go!\n");
 * else
 * 	printf("Need a DVD before I can proceed.\n");
 *
 */

/**
 * Get CDROM_DRIVE_STATUS from the kernel
 * See 'linux/cdrom.h'
 *
 * Should be used by internal functions to get drive status, and then set
 * booleans based on the return value.
 *
 */
int dvd_drive_get_status(const char *device_filename) {

	int dvd_fd;
	int drive_status;

	dvd_fd = open(device_filename, O_RDONLY);
	drive_status = ioctl(dvd_fd, CDROM_DRIVE_STATUS);
	close(dvd_fd);

	return drive_status;

}

/**
 * Check if the DVD tray IS CLOSED and HAS A DVD
 *
 */
bool dvd_drive_has_media(const char *device_filename) {

	if(dvd_drive_get_status(device_filename) == CDS_DISC_OK)
		return true;
	else
		return false;

}

/**
 * Check if the DVD tray IS OPEN
 *
 */
bool dvd_drive_is_open(const char *device_filename) {

	if(dvd_drive_get_status(device_filename) == CDS_TRAY_OPEN)
		return true;
	else
		return false;

}

/**
 * Check if the DVD tray IS CLOSED
 *
 */
bool dvd_drive_is_closed(const char *device_filename) {

	int drive_status;
	drive_status = dvd_drive_get_status(device_filename);

	if(drive_status == CDS_NO_DISC || drive_status == CDS_DISC_OK)
		return true;
	else
		return false;

}

/**
 * Check if the DVD tray IS READY TO QUERY FOR STATUS
 *
 * Should be used internally only.  All other functions run this anytime it is
 * doing a check to see if it's okay to query the status.
 *
 * This function is necessary for handling those states where a drive tray is
 * being opened or closed, and it hasn't finished initilization.  Once this
 * returns true, everything else has the green light to go.
 *
 */
bool dvd_drive_is_ready(const char* device_filename) {

	if(dvd_drive_get_status(device_filename) != CDS_DRIVE_NOT_READY)
		return true;
	else
		return false;

}

/**
 * Human-friendly print-out of the dvd drive status
 *
 */
void dvd_drive_display_status(const char *device_filename) {

	const char *status;

	switch(dvd_drive_get_status(device_filename)) {
		case 1:
			status = "no disc";
			break;
		case 2:
			status = "tray open";
			break;
		case 3:
			status = "drive not ready";
			break;
		case 4:
			status = "drive ok";
			break;
		default:
			status = "no info";
			break;
	}

	printf("%s\n", status);

}

#endif
