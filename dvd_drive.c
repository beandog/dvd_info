#ifdef __linux__
#include <stdio.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include "dvd_device.h"
#include "dvd_drive.h"

int dvd_drive_get_status(const char *device_filename) {

	int dvd_fd;
	int drive_status;

	dvd_fd = dvd_device_open(device_filename);
	drive_status = ioctl(dvd_fd, CDROM_DRIVE_STATUS);
	dvd_device_close(dvd_fd);

	return drive_status;

}

bool dvd_drive_has_media(const char *device_filename) {

	if(dvd_drive_get_status(device_filename) == CDS_DISC_OK)
		return true;
	else
		return false;

}

bool dvd_drive_is_open(const char *device_filename) {

	if(dvd_drive_get_status(device_filename) == CDS_TRAY_OPEN)
		return true;
	else
		return false;

}

bool dvd_drive_is_closed(const char *device_filename) {

	int drive_status;
	drive_status = dvd_drive_get_status(device_filename);

	if(drive_status == CDS_NO_DISC || drive_status == CDS_DISC_OK)
		return true;
	else
		return false;

}

bool dvd_drive_is_ready(const char* device_filename) {

	if(dvd_drive_get_status(device_filename) != CDS_DRIVE_NOT_READY)
		return true;
	else
		return false;

}

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
