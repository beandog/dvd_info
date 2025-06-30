#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

#define DEFAULT_DVD_DEVICE "/dev/cd0"

/**
 *      _          _        _      _                   _        _
 *   __| |_   ____| |    __| |_ __(_)_   _____     ___| |_ __ _| |_ _   _ ___
 *  / _` \ \ / / _` |   / _` | '__| \ \ / / _ \   / __| __/ _` | __| | | / __|
 * | (_| |\ V / (_| |  | (_| | |  | |\ V /  __/   \__ \ || (_| | |_| |_| \__ \
 *  \__,_| \_/ \__,_|___\__,_|_|  |_| \_/ \___|___|___/\__\__,_|\__|\__,_|___/
 *                 |_____|                   |_____|
 *
 *
 * FreeBSD tool to check if a tray has a DVD.
 *
 * This program simply opens the device with dvdread to see it is a DVD or not.
 * Getting more accurate status requires getting SCSI status of the device,
 * which is a big TODO (I can't figure it out yet).
 *
 * Exit codes:
 * 0 - ran succesfully
 * 1 - quit with errors
 * 2 - drive is closed with a DVD
 * 3 - other status, no DVD
 *
 */

bool log_verbose = false;
bool log_debug = false;

void print_usage();

void dvdread_logger_cb(void *p, dvd_logger_level_t dvdread_log_level, const char *msg, va_list dvd_log_va);

void dvd_info_logger_cb(void *p, dvd_logger_level_t dvdread_log_level, const char *msg, va_list dvd_log_va) {

	char dvd_log[2048];

	memset(dvd_log, '\0', sizeof(dvd_log));

	vsnprintf(dvd_log, sizeof(dvd_log), msg, dvd_log_va);

	if(log_verbose) {
		if(dvdread_log_level == DVD_LOGGER_LEVEL_INFO)
			fprintf(stderr, "[INFO] libdvdread: %s\n", dvd_log);
		else if(dvdread_log_level == DVD_LOGGER_LEVEL_WARN)
			fprintf(stderr, "[WARNING] libdvdread: %s\n", dvd_log);
		else if(dvdread_log_level == DVD_LOGGER_LEVEL_ERROR)
			fprintf(stderr, "[ERROR] libdvdread: %s\n", dvd_log);
	}

	if(log_debug && dvdread_log_level == DVD_LOGGER_LEVEL_DEBUG) {
		fprintf(stderr, "[DEBUG] libdvdread: %s\n", dvd_log);
	}

}

int main(int argc, char **argv) {

	int opt;
	bool exit_program = false;

	while ((opt = getopt(argc, argv, "hvz")) != -1) {

		switch(opt) {
			case 'v':
				log_verbose = true;
				break;
			case 'z':
				log_debug = true;
				break;
			case 'h':
			case '?':
				print_usage();
				exit_program = true;
				break;
			default:
				break;
		}

	}

	if(exit_program)
		return 0;

	char device_filename[PATH_MAX];
	memset(device_filename, '\0', PATH_MAX);

	if(argv[optind]) {
		strncpy(device_filename, argv[optind], PATH_MAX - 1);
	} else {
		strncpy(device_filename, DEFAULT_DVD_DEVICE, PATH_MAX - 1);
	}

	int retval;
	struct stat device_stat;

	retval = stat(device_filename, &device_stat);
	if(retval) {
		fprintf(stderr, "Could not open device %s\n", device_filename);
		return 1;
	}

	int fd;
	fd = open(device_filename, O_RDONLY | O_NONBLOCK);

	if(fd < 0) {
		fprintf(stderr, "Could not open device %s\n", device_filename);
		return 1;
	}

	/**
	 * Look to see if the disc in the drive is a DVD or not.
	 * libdvdread will open any disc just fine, so scan it to see if there
	 * is a VMG IFO as well as an additional check.
	 */
	dvd_reader_t *dvdread_dvd = NULL;

	dvd_logger_cb dvdread_logger_cb = { dvd_info_logger_cb };
	
	dvdread_dvd = DVDOpen2(NULL, &dvdread_logger_cb, device_filename);
	if(dvdread_dvd) {

		ifo_handle_t *vmg_ifo = NULL;
		
		vmg_ifo = ifoOpen(dvdread_dvd, 0);

		if(vmg_ifo == NULL) {
			ifoClose(vmg_ifo);
			DVDClose(dvdread_dvd);
			close(fd);
			printf("no dvd\n");
			return 3;
		} else {
			printf("has dvd\n");
			DVDClose(dvdread_dvd);
			close(fd);
			return 2;
		}

	}

	close(fd);

	return 0;

}

void print_usage(void) {

	printf("dvd_drive_status [options] [device]\n");
	printf("  -h     Display this help output\n");
	printf("  -v     Display verbose output\n");
	printf("\n");
	printf("Default device is %s\n", DEFAULT_DVD_DEVICE);

}
