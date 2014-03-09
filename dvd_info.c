#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/dvd_udf.h>
#include <dvdread/ifo_read.h>

#include <string.h>

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

	// libdvdread

	// open DVD device and don't cache queries
	dvd_reader_t *dvd;
	dvd = DVDOpen(device_filename);
	DVDUDFCacheLevel(dvd, 0);

	ifo_handle_t *ifo_zero;
	ifo_zero = ifoOpen(dvd, 0);
	if(!ifo_zero) { fprintf(stderr, "opening ifo_zero failed\n"); return 1; }

	int nr_of_vtss = ifo_zero->vts_atrt->nr_of_vtss;
	printf("nr_of_vtss: %i\n", nr_of_vtss);

	ifoClose(ifo_zero);
	DVDClose(dvd);

}
