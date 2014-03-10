#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <dvdread/dvd_reader.h>

int main(int argc, char **argv) {

	int cdrom;
	int drive_status;
	int device_is_dvd_drive = 1;
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
	if(drive_status == 0) {
		device_is_dvd_drive = 0;
	}
	close(cdrom);

	if(device_is_dvd_drive == 0) {
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
	}

	// begin libdvdread
	dvd_reader_t *dvd;
	dvd = DVDOpen(device_filename);
	if(dvd == 0) {
		fprintf(stderr, "libdvdread could not open %s\n", device_filename);
		return 1;
	}

	int dvd_disc_id;
	unsigned char tmp_buf[16];

	dvd_disc_id = DVDDiscID(dvd, tmp_buf);

	if(dvd_disc_id == -1) {
		fprintf(stderr, "libdvdread: DVDDiscID() failed\n");
		return 1;
	}

	for(int x = 0; x < sizeof(tmp_buf); x++) {
		printf("%02x", tmp_buf[x]);
	}
	printf("\n");

	DVDClose(dvd);

	return 0;

}
