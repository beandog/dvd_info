#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>

/**
 * dvd_title.c
 * A simple little program to get the DVD title
 *
 * Checks if the device is a DVD drive or not, and if it is, also looks to see
 * if it can be polled or not.
 */

int main(int argc, char **argv) {

	int cdrom;
	int drive_status;
	char* dvd_device;
	char title[33];
	FILE* filehandle = 0;
	int x, y, z;

	if(argc == 1)
		dvd_device = "/dev/dvd";
	else
		dvd_device = argv[1];

	// Check if device exists
	if(access(dvd_device, F_OK) != 0) {
		fprintf(stderr, "cannot access %s\n", dvd_device);
		return 1;
	}

	// Open device
	cdrom = open(dvd_device, O_RDONLY | O_NONBLOCK);
	if(cdrom < 0) {
		fprintf(stderr, "error opening %s\n", dvd_device);
		return 1;
	}

	drive_status = ioctl(cdrom, CDROM_DRIVE_STATUS);

	// If the device is a DVD drive, then wait up to 30 seconds for it to be ready (not opening or closing),
	// and then check if there is a disc in the tray.  If there's no disc, exit quietly.
	// If the drive is open, then it just quietly exits.
	if(drive_status > 0) {
		int max_sleepy_time = 30;
		int num_sleepy_times = 0;
		while(ioctl(cdrom, CDROM_DRIVE_STATUS) == CDS_DRIVE_NOT_READY && num_sleepy_times < max_sleepy_time) {
			sleep(1);
			num_sleepy_times++;
		}
		if(ioctl(cdrom, CDROM_DRIVE_STATUS) != CDS_DISC_OK) {
			close(cdrom);
			return 1;
		}
	}

	close(cdrom);

	filehandle = fopen(dvd_device, "r");
	if(filehandle == NULL) {
		fprintf(stderr, "could not open device %s for reading\n", dvd_device);
		return 1;
	}

	if(fseek(filehandle, 32808, SEEK_SET) == -1) {
		fprintf(stderr, "could not seek on device %s\n", dvd_device);
		fclose(filehandle);
		return 1;
	}

	x = fread(title, 1, 32, filehandle);
	if(x == 0) {
		fprintf(stderr, "could not read device %s\n", dvd_device);
		fclose(filehandle);
		return 1;
	}
	title[32] = '\0';

	fclose(filehandle);

	y = sizeof(title);
	while(y-- > 2) {
		if(title[y] == ' ') {
			title[y] = '\0';
		}
	}

	for(z = 0; z < strlen(title); z++) {
		printf("%c", title[z]);
	}
	printf("\n");

	return 0;
}
