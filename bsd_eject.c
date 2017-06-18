#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/cdio.h>
#include <sys/ioctl.h>

#if defined (__FreeBSD__)
#define DEFAULT_DVD_DEVICE "/dev/cd0"
#elif defined (__NetBSD__)
#define DEFAULT_DVD_DEVICE "/dev/rcd0d"
#elif defined (__OpenBSD__)
#define DEFAULT_DVD_DEVICE "/dev/rcd0c"
#else
#define DEFAULT_DVD_DEVICE "/dev/dvd"
#endif

void print_usage(void);

int main(int argc, char **argv) {

	int opt;
	bool eject_device = true;
	while ((opt = getopt(argc, argv, "ht")) != -1) {

		switch(opt) {
			case 't':
				eject_device = false;
				break;
			case 'h':
			case '?':
				print_usage();
				return 0;
		}

	}

	char device_filename[11] = {'\0'};
	if(argv[optind]) {
		strncpy(device_filename, argv[optind], 10);
	} else {
		strncpy(device_filename, DEFAULT_DVD_DEVICE, 10);
	}

	int cdrom = -1;
        cdrom = open(device_filename, O_RDONLY);

	if(cdrom < 0) {
		if(errno == EACCES) {
			printf("Permission denied accessing %s\n", device_filename);
		} else if(errno == ENODEV) {
			printf("Operation not supported on device %s\n", device_filename);
		} else {
			printf("Could not open device %s\n", device_filename);
		}
		if(strncmp(device_filename, DEFAULT_DVD_DEVICE, 10))
			printf("Possible invalid device name? Try %s instead\n", DEFAULT_DVD_DEVICE);
		return 1;
	}

        if(eject_device) {

		int retval = 0;
		retval = ioctl(cdrom, CDIOCEJECT);

		if(retval < 0) {
			printf("retval: %i\n", retval);
			printf("errno: %i\n", errno);
			close(cdrom);
			return 1;
		}

	}
	

	close(cdrom);

        return 0;

}

void print_usage(void) {

	printf("dvd_eject [options] [device]\n");
	printf("	-h	this help output\n");
	printf("	-t	close tray instead of opening\n");
	printf("\n");
	printf("Default DVD device is %s\n", DEFAULT_DVD_DEVICE);

}
