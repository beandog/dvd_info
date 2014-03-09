#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <dvdnav/dvdnav.h>

int main(int argc, char **argv) {

	int cdrom;
	int drive_status;
	char* device_filename;
	char* status;

	if(argc == 1)
		device_filename = "/dev/dvd";
	else
		device_filename = argv[1];

	// Check if device exists
	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "cannot access %s\n", device_filename);
		return 1;
	}

	cdrom = open(device_filename, O_RDONLY | O_NONBLOCK);
	if(cdrom < 0) {
		fprintf(stderr, "error opening %s\n", device_filename);
		return 1;
	}
	drive_status = ioctl(cdrom, CDROM_DRIVE_STATUS);
	if(drive_status < 0) {
		fprintf(stderr, "%s is not a DVD drive\n", device_filename);
		close(cdrom);
		return 1;
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

	if(drive_status != CDS_DISC_OK) {
		printf("%s\n", status);
		return 1;
	}

	// begin libdvdnav
	dvdnav_t *dvdnav;
	int ret;

	ret = dvdnav_open(&dvdnav, device_filename);
	if(ret != DVDNAV_STATUS_OK) {
		fprintf(stderr, "libdvdnav dvdnav_open(%s) failed\n", device_filename);
		return 1;
	}

	int32_t num_titles;

	ret = dvdnav_get_number_of_titles(dvdnav, &num_titles);
	if(ret != DVDNAV_STATUS_OK) {
		fprintf(stderr, "libdvdnav dvdnav_get_number_of_titles(%s) failed\n", device_filename);
		dvdnav_close(dvdnav);
		return 1;
	}
	printf("num titles: %i\n", num_titles);

	dvdnav_close(dvdnav);

	return 0;

}
