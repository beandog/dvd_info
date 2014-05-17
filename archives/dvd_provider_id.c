#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

/**
 * dvd_provider_id.c
 * A simple little program to get the DVD title
 *
 * The DVD provider id can be used to help uniquely identify a DVD.  It is a
 * string with a max length of 32 characters, but sometimes is empty.  This
 * string is not enough to create an unique identifier on its own, and is best
 * used in conjunction with other unique properties of a DVD.
 */

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
		fprintf(stderr, "%s\n", status);
		return 1;
	}

	// begin libdvdread
	dvd_reader_t *dvd;
	dvd = DVDOpen(device_filename);
	if(dvd == 0) {
		fprintf(stderr, "libdvdread could not open %s\n", device_filename);
		return 1;
	}

	ifo_handle_t *vmg_ifo;
	vmg_ifo = ifoOpen(dvd, 0);

	if(!vmg_ifo) {
		fprintf(stderr, "libdvdread: ifoOpen() failed\n");
		return 1;
	}

	char *provider_id;
	provider_id = vmg_ifo->vmgi_mat->provider_identifier;
	printf("%s\n", provider_id);

	ifoClose(vmg_ifo);

	DVDClose(dvd);

	return 0;

}
