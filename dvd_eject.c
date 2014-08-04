#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <dvdcss/dvdcss.h>
// #include <dvdread/dvd_reader.h>

#define DEFAULT_DVD_DEVICE "/dev/dvd"

int drive_status(const int cdrom);
bool has_media(const int cdrom);
bool is_open(const int cdrom);
bool is_closed(const int cdrom);
bool is_ready(const int cdrom);
int open_tray(const int cdrom);
int close_tray(const int cdrom);
int unlock_door(const int cdrom);
int main(int argc, char **argv);

int main(int argc, char **argv) {

	char *str_options;
	int long_index;
	int opt;
	opterr = 1;
	__useconds_t sleepy_time;
	int cdrom;
	char *device_filename;
	bool eject_open, eject_close;
	bool tray_open;
	bool tray_has_media;
	bool retry;
	bool wait;
	int retval;
	bool d_help;
	// dvdcss_t *dvdcss;
	// dvd_reader_t *dvdread_dvd;

	struct option long_options[] = {
		{ "close", no_argument, 0, 't' },
		{ 0, 0, 0, 0 }
	};

	str_options = "ghrt";
	sleepy_time = 1000000;
	eject_open = true;
	eject_close = false;
	tray_open = false;
	tray_has_media = false;
	retry = false;
	wait = true;
	retval = 0;
	d_help = false;

	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {
		switch(opt) {
			case 'g':
				wait = false;
				break;
			case 'h':
				d_help = true;
				break;
			case 'r':
				retry = true;
				break;
			case 't':
				eject_open = false;
				eject_close = true;
				break;
			default:
				break;
		}
	}

	if(d_help) {
		printf("dvd_eject [options] [device]\n");
		printf("\t-h\tthis help output\n");
		printf("\t-t\tclose tray instead of opening\n");
		printf("\t-w\twait for device to become ready after opening / closing (default)\n");
		printf("\t-g\tdo not wait for device to become ready\n");
		printf("\t-r\tkeep retrying to open / close a device\n");
		printf("\t-q\tdo not display progress\n");
		return 0;
	}

	if(argv[optind])
		device_filename = argv[optind];
	else
		device_filename = DEFAULT_DVD_DEVICE;

	cdrom = open(device_filename, O_RDONLY | O_NONBLOCK);

	if(cdrom < 0) {
		printf("error opening %s\n", device_filename);
		return 1;
	}

	// Fetch status
	if(drive_status(cdrom) < 0) {

		printf("%s is not a DVD drive\n", device_filename);
		close(cdrom);
		return 1;

	}

	if(eject_open)
		printf("[Open Drive Tray]\n");
	if(eject_close)
		printf("[Close Drive Tray]\n");
	printf("* Device: %s\n", device_filename);

	// Wait for the device to be ready before performing any actions
	while(is_ready(cdrom) == false) {
		printf("* Prepping crew, sir.\n");
		usleep(sleepy_time);
	}
	printf("* The bridge is now at your command.\n");

	tray_open = is_open(cdrom);
	tray_has_media = has_media(cdrom);

	if(tray_open)
		printf("* Docking port open.\n");
	else {
		printf("* Docking port closed.\n");
		if(tray_has_media)
			printf("* Has media\n");
		else
			printf("* No media\n");
	}

	// Check for silly users
	if(eject_open && tray_open) {

		printf("* Are you lost, captain?\n");
		close(cdrom);
		return 0;

	}
	if(eject_close && !tray_open) {

		printf("* Are you sure you belong on the bridge, sir?\n");

		if(has_media(cdrom)) {
			printf("* Scanning Deck C section 55 for anomalies ... ");
			if(dvdcss_open(device_filename) == NULL) {
				printf("red alert!!\n");
			} else {
				printf("all systems go!\n");
				return 2;
			}
		}

		close(cdrom);
		return 0;

	}

	// Open the tray as requested, if it is closed
	if(eject_open && !tray_open) {

		unlock_door(cdrom);

		printf("* Leaving space dock ...\n");
		retval = open_tray(cdrom);

		while((wait == true) && (is_ready(cdrom) == false)) {
			printf("* Awaiting clearance for warp speed, captain.\n");
			usleep(sleepy_time);
		}
		printf("* Blast off!\n");

		// Do one last nap
		usleep(1000000);

		close(cdrom);

		return 0;

	}

	// Close the tray as requested, if it is open
	if(eject_close && tray_open) {

		printf("* Shuttle cleared for docking!\n");
		retval = close_tray(cdrom);

		if(retval != 0) {
			printf("* Emergency cleanup on deck twelve! :(\n");
			close(cdrom);
			return 1;
		}

		while((wait == true) && (is_ready(cdrom) == false)) {
			printf("* Steady as she goes ...\n");
			usleep(sleepy_time);
		}

		// Open DVD device
		if(has_media(cdrom)) {
			printf("* Scanning Deck C section 55 for anomalies ... ");
			if(dvdcss_open(device_filename) == NULL)
				printf("red alert!!\n");
			else
				printf("all systems go!\n");
		}

		// Open DVD device
		/*
		printf("* Opening device with dvdread ... ");
		dvdread_dvd = DVDOpen(device_filename);
		if(!dvdread_dvd) {
			printf("failed\n");
		} else {
			printf("ok\n");
		}
		*/

		printf("* Welcome to the station! Enjoy your stay.\n");

		close(cdrom);

		return 0;

	}

	close(cdrom);

	return 0;

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

	return ioctl(cdrom, CDROMEJECT);

}

int close_tray(const int cdrom) {

	return ioctl(cdrom, CDROMCLOSETRAY);

}

int unlock_door(const int cdrom) {

	return ioctl(cdrom, CDROM_LOCKDOOR, 0);

}

