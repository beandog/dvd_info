#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <dvdcss/dvdcss.h>

#define DEFAULT_DVD_DEVICE "/dev/sr0"

/**
 * dvd_eject.c
 *
 * Opens / closes a disc tray, and decrypts the CSS on the drive to avoid
 * any spatial anomalies (and hardware devices whining to the kernel about
 * reading sectors without authentication).  Requires libdvdcss as a
 * dependency library.
 *
 * Three return values: 0 on success, 1 on error, 2 on success and drive has
 * a DVD inside of it.
 *
 * To build:
 * $ gcc -o dvd_eject dvd_eject.c -l dvdcss
 *
 * Usage:
 * $ dvd_eject -h
 *
 * Story mode:
 *
 * I've spent a lot of time trying to track down why the hardware will send
 * errors to the systemlog like so:
 *
 * "Sense Key : Illegal Request [current]"
 * "Add. Sense: Read of scrambled sector without authentication"
 *
 * I'm debugging them not because I care about the messages, but becaue if it
 * happens enough, the drives will eventually lock up and be unable to process
 * any more discs (until next reboot).
 *
 * I've narrowed down two things that help a lot to prevent them as best as I
 * can:
 *
 * 1. Make as *few* calls as possible to the device to check on its status
 *    (this program does that -- it keeps it to a bare minimum)
 * 2. Use libdvdcss to authenticate access to the DVD.
 *
 * Those two combined seem to help in large amounts, but I've never found
 * anything that is a sure fire solution.
 *
 * The basic logic of this program and reason for it is this: Opening and
 * closing a disc tray is trivial.  The issue this one works around, however,
 * is that just because a tray is *closed*, does not mean that it is *ready* to
 * acess the media in it.  This can cause issues when you do something like
 * "lsdvd /dev/dvd" and the tray is open.  The tray will close by the call made
 * through libdvdread / libdvdcss to access the device, but will fail because
 * the device is not yet in a ready mode.  So there's a gap between "closing" and
 * "closed and ready".  Fortunately, the Linux kernel allows for checking those
 * states, and that's the core of the logic here.
 *
 * With that in mind, all this does is close or open the tray, wait until it is
 * a "ready" state again, and then exits.  In addition, if it closes the tray,
 * it waits until the "ready" state and then decrypts the CSS using libdvdcss.
 *
 * If anecdotal evidence has any value ... it works for me. :)
 *
 * Good luck, and here's to all the other DVD collectors / multimedia geeks out
 * there! :D
 *
 */

int drive_status(const int dvd_fd);
bool has_media(const int dvd_fd);
bool is_open(const int dvd_fd);
bool is_ready(const int dvd_fd);
int open_tray(const int dvd_fd);
int close_tray(const int dvd_fd);
int unlock_door(const int dvd_fd);

int main(int argc, char **argv) {

	int long_index = 0;
	int opt = 0;
	opterr = 1;
	uint32_t sleepy_time = 1000000;
	int dvd_fd = -1;
	char *device_filename;
	int starbase = 51;
	bool p_dvd_eject = true;
	bool p_dvd_close = false;
	bool dvd_drive_opened = false;
	bool dvd_drive_has_media = false;
	// bool opt_retry = false;
	bool opt_wait = true;
	int max_waiting_times = 15;
	int times_waited = 0;
	int retval = -1;
	bool d_help = false;
	// dvdcss_t *dvdcss;
	// dvd_reader_t *dvdread_dvd;

	struct option long_options[] = {
		{ "close", no_argument, 0, 't' },
		{ "help", no_argument, 0, 'h' },
		{ "no-wait", no_argument, 0, 'n' },
		// { "retry", no_argument, 0, 'r' },
		{ 0, 0, 0, 0 }
	};

	while((opt = getopt_long(argc, argv, "hnrt", long_options, &long_index )) != -1) {
		switch(opt) {
			case 'h':
				d_help = true;
				break;
			case 'n':
				opt_wait = false;
				break;
			// case 'r':
			//	opt_retry = true;
			//	break;
			case 't':
				p_dvd_eject = false;
				p_dvd_close = true;
				break;
			case '?':
				d_help = true;
				break;
			case 0:
			default:
				break;
		}
	}

	if(d_help) {
		printf("dvd_eject [options] [device]\n\n");
		printf("-h, --help	Display this help output\n");
		printf("-t, --close	Close tray\n");
		printf("-n, --no-wait	Don't wait for device to be ready when closing\n");
		// printf("-r, --retry	Keep retrying to open / close a tray\n");
		printf("\nDefault device is %s\n", DEFAULT_DVD_DEVICE);
		return 0;
	}

	if(argv[optind])
		device_filename = argv[optind];
	else
		device_filename = DEFAULT_DVD_DEVICE;

	if(strlen(device_filename) == 11 && strcmp("/dev/bluray", device_filename) == 0) {
		starbase = 71;
	}

	dvd_fd = open(device_filename, O_RDONLY | O_NONBLOCK);

	if(dvd_fd < 0) {
		printf("error opening %s\n", device_filename);
		return 1;
	}

	// Fetch status
	if(drive_status(dvd_fd) < 0) {

		printf("%s is not a DVD drive\n", device_filename);
		close(dvd_fd);
		return 1;

	}

	if(p_dvd_eject)
		printf("[Open Drive Tray]\n");
	if(p_dvd_close)
		printf("[Close Drive Tray]\n");
	printf("* Device: %s\n", device_filename);

	if(opt_wait == false && is_ready(dvd_fd) == false) {
		printf("* No waiting requested, and device is not ready. Exiting\n");
		close(dvd_fd);
		return 0;
	}

	printf("* Prepping crew, sir ...");
	// Wait for the device to be ready before performing any actions
	while(is_ready(dvd_fd) == false) {
		printf(".");
		usleep(sleepy_time);
		times_waited += 1;
		if(times_waited == max_waiting_times) {
			printf("\n");
			printf("* Waited %i, tired of waiting, trying workarounds\n", max_waiting_times);
			printf("* Closing and reopening device\n");
			if(close(dvd_fd) == 0) {
				printf("* Closing file descriptor worked\n");
			} else {
				printf("* Closing file descriptor failed, continuing anyway\n");
			}
			printf("* Reopening file descriptor\n");
			dvd_fd = open(device_filename, O_RDONLY | O_NONBLOCK);
			if(dvd_fd == 0) {
				printf("* Closing and reopening file descriptor worked, checking if device is ready now\n");
				if(is_ready(dvd_fd) == false) {
					printf("* Drive still not marked as ready, quitting\n");
					close(dvd_fd);
					return 1;
				}
				printf("* Opening file descriptor failed, exiting\n");
				return 1;
			}
		}
	}

	printf(" at your command!\n");

	dvd_drive_opened = is_open(dvd_fd);
	dvd_drive_has_media = has_media(dvd_fd);

	if(dvd_drive_opened)
		printf("* Docking port status: open\n");
	else {
		printf("* Docking port status: closed\n");
		if(!dvd_drive_has_media)
			printf("* Shuttle bay empty\n");
	}

	// Check for silly users
	if(p_dvd_eject && dvd_drive_opened) {
	//	printf("* Are you sure you belong on the bridge, sir?\n");
		close(dvd_fd);
		return 0;
	}

	if(p_dvd_close && !dvd_drive_opened) {

		if(has_media(dvd_fd)) {
			printf("* Scanning Deck C section 55 for anomalies ... ");
			if(dvdcss_open(device_filename) == NULL) {
				printf("red alert!!\n");
			} else {
				printf("all systems go!\n");
				return 2;
			}
		}

		close(dvd_fd);
		return 0;

	}

	// Open the tray as requested, if it is closed
	if(p_dvd_eject && !dvd_drive_opened) {

		unlock_door(dvd_fd);

		printf("* Leaving space dock ...\n");
		retval = open_tray(dvd_fd);

		printf("* Awaiting clearance for warp speed, captain ...");
		if(opt_wait) {
			while(!is_ready(dvd_fd)) {
				printf(".");
				usleep(sleepy_time);
			}
		}

		// Do one last nap
		usleep(sleepy_time);

		printf(" engaging at warp 5.1 sir!\n");

		close(dvd_fd);

		return 0;

	}

	// Close the tray as requested, if it is open
	if(p_dvd_close && dvd_drive_opened) {

		printf("* Shuttle cleared for docking!\n");
		retval = close_tray(dvd_fd);

		if(retval != 0) {
			printf("* Emergency cleanup on deck twelve! :(\n");
			close(dvd_fd);
			return 1;
		}

		if(opt_wait) {
			while(!is_ready(dvd_fd)) {
				printf("* Steady as she goes ...\n");
				usleep(sleepy_time);
			}
		}

		// Open DVD device
		if(has_media(dvd_fd)) {
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

		close(dvd_fd);

		return 0;

	}

	close(dvd_fd);

	return 0;

}

int drive_status(const int dvd_fd) {

	return ioctl(dvd_fd, CDROM_DRIVE_STATUS);

}

bool has_media(const int dvd_fd) {

	if(drive_status(dvd_fd) == CDS_DISC_OK)
		return true;
	else
		return false;

}

bool is_open(const int dvd_fd) {

	if(drive_status(dvd_fd) == CDS_TRAY_OPEN)
		return true;
	else
		return false;

}

bool is_ready(const int dvd_fd) {

	if(drive_status(dvd_fd) != CDS_DRIVE_NOT_READY)
		return true;
	else
		return false;

}

int open_tray(const int dvd_fd) {

	return ioctl(dvd_fd, CDROMEJECT);

}

int close_tray(const int dvd_fd) {

	return ioctl(dvd_fd, CDROMCLOSETRAY);

}

int unlock_door(const int dvd_fd) {

	return ioctl(dvd_fd, CDROM_LOCKDOOR, 0);

}

