#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <getopt.h>

#define DEFAULT_DVD_DEVICE "/dev/dvd"

int open_device(const char *device) {

	return open(device, O_RDONLY | O_NONBLOCK);

}

int drive_status(const int cdrom) {

	return ioctl(cdrom, CDROM_DRIVE_STATUS);

}

bool has_media(const int cdrom) {

	if(drive_status(cdrom) == CDS_DISC_OK)
		return true;
	else
		return false;

}

bool is_open(const int cdrom) {

	if(drive_status(cdrom) == CDS_TRAY_OPEN)
		return true;
	else
		return false;

}

bool is_closed(const int cdrom) {

	int status = drive_status(cdrom);

	if(status == CDS_NO_DISC || status == CDS_DISC_OK)
		return true;
	else
		return false;

}

bool is_ready(const int cdrom) {

	if(drive_status(cdrom) != CDS_DRIVE_NOT_READY)
		return true;
	else
		return false;

}

int open_tray(const int cdrom) {

	return(ioctl(cdrom, CDROMEJECT));

}

int close_tray(const int cdrom) {

	return(ioctl(cdrom, CDROMCLOSETRAY));

}

int unlock_door(const int cdrom) {

	return ioctl(cdrom, CDROM_LOCKDOOR, 0);

}


int main(int argc, char **argv) {

	char *str_options;
	int long_index;
	int opt;
	opterr = 1;
	__useconds_t sleepy_time;
	int cdrom;
	char *device_filename;
	int status;
	bool eject;
	bool retry;
	bool verbose;
	int retval;

	struct option long_options[] = {
		{ "close", no_argument, 0, 't' },
		{ 0, 0, 0, 0 }
	};

	str_options = "frtv";
	sleepy_time = 1000000;
	eject = true;
	retry = false;
	verbose = false;
	retval = 0;

	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {
		switch(opt) {
			case 'r':
				retry = true;
				break;
			case 't':
				eject = false;
				break;
			case 'v':
				verbose = true;
				break;
			default:
				break;
		}
	}

	if(argv[optind])
		device_filename = argv[optind];
	else
		device_filename = DEFAULT_DVD_DEVICE;

	cdrom = open_device(device_filename);

	if(cdrom < 0) {
		fprintf(stderr, "error opening %s\n", device_filename);
		return 1;
	}

	// Fetch status
	if(drive_status(cdrom) < 0) {

		fprintf(stderr, "%s is not a DVD drive\n", device_filename);
		close(cdrom);
		return 1;

	}

	if(verbose) {
		printf("[Eject Drive]\n");
		printf("* Device: %s\n", device_filename);
	}

	// Wait for the device to be ready before performing any actions
	if(is_ready(cdrom) == false) {

		if(verbose && !is_ready(cdrom))
			printf("* Waiting\n");

		while(is_ready(cdrom) == false)
			usleep(sleepy_time);

	}

	status = drive_status(cdrom);

	if(verbose) {

		if(is_open(cdrom))
			printf("* Opened\n");
		else if(is_closed(cdrom)) {
			printf("* Closed\n");
			if(has_media(cdrom))
				printf("* Has media\n");
			else
				printf("* No media\n");
		}

	}


	if(eject && is_closed(cdrom)) {

		unlock_door(cdrom);

		if(verbose)
			printf("* Eject!\n");
		retval = open_tray(cdrom);

		if(retval != 0) {
			while(retval != 0) {
				printf("* Retrying\n");
				retval = open_tray(cdrom);
			}
		}

	} else if(!eject && is_open(cdrom)) {

		if(verbose)
			printf("* Closing\n");
		retval = close_tray(cdrom);

		if(retval != 0) {

			fprintf(stderr, "* Closing failed\n");
			close(cdrom);
			return 1;

		}

	}

	if(!is_ready(cdrom)) {

		while(!is_ready(cdrom)) {
			if(verbose)
				printf("* Waiting\n");
			usleep(sleepy_time);
		}

	}

	if(verbose)
		printf("* Ready\n");

	close(cdrom);

	return 0;
}
