#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <inttypes.h>
#include <linux/cdrom.h>
#include <linux/limits.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

/**
 * dvd_backup_ifo - creates a copy of a DVD's VMG and VTS .IFO and .BUP files
 * which can be used for debugging by developers. Requires libdvdread.
 *
 * @author Steve Dibb <steve.dibb@gmail.com>
 * @license GPLv2
 * @site http://dvds.beandog.org/
 * @site https://github.com/beandog/dvd_info
 *
 * To build:
 * $ cc -o dvd_backup_ifo dvd_backup_ifo.c -l dvdread
 *
 * Usage:
 * $ dvd_backup_ifo [DVD path]
 * "DVD path" can be a device filename, single file, or directory.
 * Creates a "VIDEO_TS" directory in the current path.
 *
 * Examples:
 * $ dvd_backup_ifo /dev/sr0
 * $ dvd_backup_ifo BORN_WHEE.iso
 * $ dvd_backup_ifo CAPTAIN_HACKIT/
 *
 * Story mode:
 *
 * The VMG and VTS IFOs store all the metadata about a DVD. From a development
 * point-of-view, if a user is having a problem reading the DVD with userland
 * tools, then they need a way to get a copy to upstream without providing the
 * entire image.
 *
 * The .IFO and .BUP files are really small, and will probably end up
 * totalling between 500kB and 2 MBs. These files can be tarballed / zipped
 * up and sent off to upstream of a DVD application to debug why they are
 * not being read. That's it! Super easy. :)
 *
 */

#define DEFAULT_DVD_DEVICE "/dev/dvd"

int main(int, char **);

int main(int argc, char **argv) {

	int dvd_fd;
	int drive_status;
	uint16_t num_ifos = 1;
	uint16_t ifo_number = 0;
	bool is_hardware = false;
	const char *device_filename = DEFAULT_DVD_DEVICE;
	dvd_reader_t *dvdread_dvd;
	ifo_handle_t *ifo;
	dvd_file_t *dvdread_ifo_file;
	ssize_t ifo_filesize;
	uint8_t *buffer = NULL;
	ssize_t bytes_read;
	ssize_t ifo_bytes_written;
	ssize_t bup_bytes_written;
	int streamout_ifo = -1, streamout_bup = -1;
	char ifo_filename[PATH_MAX] = {'\0'};
	char bup_filename[PATH_MAX] = {'\0'};
	bool directory_exists = false;
	int z = 1;

	if (argc > 1) {
		device_filename = argv[1];
	}

	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "cannot access %s\n", device_filename);
		return 1;
	}

	dvd_fd = open(device_filename, O_RDONLY | O_NONBLOCK);
	if(dvd_fd < 0) {
		fprintf(stderr, "error opening %s\n", device_filename);
		return 1;
	}
	drive_status = ioctl(dvd_fd, CDROM_DRIVE_STATUS);
	close(dvd_fd);

	// Poll drive status if it is hardware
	if(strncmp(device_filename, "/dev/", 5) == 0)
		is_hardware = true;

	if(is_hardware) {

		if(drive_status != CDS_DISC_OK) {

			switch(drive_status) {
				case 1:
					fprintf(stderr, "drive status: no disc");
					break;
				case 2:
					fprintf(stderr, "drive status: tray open");
					break;
				case 3:
					fprintf(stderr, "drive status: drive not ready");
					break;
				default:
					fprintf(stderr, "drive status: unable to poll");
					break;
			}

			return 1;
		}
	}

	printf("[DVD]\n");
	printf("* Opening device %s\n", device_filename);

	dvdread_dvd = DVDOpen(device_filename);

	if(!dvdread_dvd) {
		printf("* dvdread could not open %s\n", device_filename);
		return 1;
	}

	printf("* Opening IFO zero\n");
	ifo = ifoOpen(dvdread_dvd, 0);

	if(!ifo) {
		printf("* Could not open IFO zero\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	num_ifos = ifo->vts_atrt->nr_of_vtss;

	printf("* %d title IFOs present\n", num_ifos);

	if(num_ifos < 1) {
		printf("* DVD has no title IFOs?!\n");
		printf("* Most likely a bug in libdvdread or a bad master or problems reading the disc\n");
		ifoClose(ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}

	if(ifo) {
		ifoClose(ifo);
		ifo = NULL;
	}

	// Check if VIDEO_TS directory exists
	DIR *dir = opendir("VIDEO_TS");
	if(dir) {
		directory_exists = true;
		printf("* VIDEO_TS directory already exists\n");
		return 1;
	}

	char video_ts_filenames[20][22];
	snprintf(video_ts_filenames[0], 22, "VIDEO_TS/VIDEO_TS.IFO");
	snprintf(video_ts_filenames[1], 22, "VIDEO_TS/VIDEO_TS.BUP");

	for(z = 1; z < 10; z++) {
		snprintf(video_ts_filenames[z + 1], 22, "VIDEO_TS/VTS_%02i_0.IFO", z);
		snprintf(video_ts_filenames[z + 2], 22, "VIDEO_TS/VTS_%02i_0.BUP", z);
	}

	int mkdir_retval;
	mkdir_retval = mkdir("VIDEO_TS", 0755);

	if(mkdir_retval == -1) {
		printf("* Could not create VIDEO_TS directory\n");
		return 1;
	}

	// Open IFO directly
	// See DVDCopyIfoBup() in dvdbackup.c for reference
	for (ifo_number = 0; ifo_number < num_ifos + 1; ifo_number++) {

		printf("[IFO %d]\n", ifo_number);

		ifo = ifoOpen(dvdread_dvd, ifo_number);

		// TODO work around broken IFOs by copying contents directly to filesystem
		if(!ifo) {
			printf("* libdvdread ifoOpen() %i FAILED\n", ifo_number);
			printf("* Skipping IFO\n");
			continue;
		}

		dvdread_ifo_file = DVDOpenFile(dvdread_dvd, ifo_number, DVD_READ_INFO_FILE);

		if(dvdread_ifo_file == 0) {
			printf("* libdvdread DVDOpenFile() %i FAILED\n", ifo_number);
			printf("* Skipping IFO\n");
			ifoClose(ifo);
			ifo = NULL;
			continue;
		}

		ifo_filesize = DVDFileSize(dvdread_ifo_file) * DVD_VIDEO_LB_LEN;
		printf("* Block filesize: %ld\n", ifo_filesize);

		// Allocate enough memory for the buffer, *now that we know the filesize*
		// printf("* Allocating buffer\n");
		buffer = (uint8_t *)malloc((unsigned long)ifo_filesize * sizeof(uint8_t));

		if(buffer == NULL) {
			printf("* Could not allocate memory for buffer\n");
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		// printf("* Seeking to beginning of file\n");
		DVDFileSeek(ifo->file, 0);

		// Need to check to make sure it could read the right size
		printf("* Reading DVD bytes\n");
		bytes_read = DVDReadBytes(ifo->file, buffer, (size_t)ifo_filesize);
		if(bytes_read != ifo_filesize) {
			printf(" * Bytes read and IFO filesize do not match: %ld, %ld\n", bytes_read, ifo_filesize);
			ifoClose(ifo);
			ifo = NULL;
			continue;
		}

		// TODO
		// * Create VIDEO_TS file if it does not exist
		// * Create IFO, BUP files if we have permission, etc.
		// See http://linux.die.net/man/3/stat
		// See http://linux.die.net/man/3/mkdir

		if(ifo_number == 0) {
			snprintf(ifo_filename, 27, "VIDEO_TS/VIDEO_TS.IFO");
			snprintf(bup_filename, 27, "VIDEO_TS/VIDEO_TS.BUP");
		} else {
			snprintf(ifo_filename, 27, "VIDEO_TS/VTS_%02i_0.IFO", ifo_number);
			snprintf(bup_filename, 27, "VIDEO_TS/VTS_%02i_0.BUP", ifo_number);
		}

		// file handlers
		printf("* Creating .IFO\n");
		streamout_ifo = open(ifo_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		printf("* Creating .BUP\n");
		streamout_bup = open(bup_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(streamout_ifo == -1) {
			printf("* Could not create %s\n", ifo_filename);
			free(buffer);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}
		if(streamout_bup == -1) {
			printf("* Could not open %s\n", bup_filename);
			free(buffer);
			close(streamout_ifo);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		printf("* Writing to %s\n", ifo_filename);
		ifo_bytes_written = write(streamout_ifo, buffer, (size_t)ifo_filesize);
		printf("* Writing to %s\n", bup_filename);
		bup_bytes_written = write(streamout_bup, buffer, (size_t)ifo_filesize);

		// Check that source size and target sizes match
		if((ifo_bytes_written != ifo_filesize) || (bup_bytes_written != ifo_filesize) || (ifo_bytes_written != bup_bytes_written)) {
			if(ifo_bytes_written != ifo_filesize)
				printf("* IFO num bytes written and IFO filesize do not match: %ld, %ld\n", ifo_bytes_written, ifo_filesize);
			if(bup_bytes_written != ifo_filesize)
				printf("* BUP num bytes written and BUP filesize do not match: %ld, %ld\n", bup_bytes_written, ifo_filesize);
			if(ifo_bytes_written != bup_bytes_written)
				printf("* IFO num bytes written and BUP num bytes written do not match: %ld, %ld\n", ifo_bytes_written, bup_bytes_written);
			free(buffer);
			close(streamout_ifo);
			close(streamout_bup);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		// TODO: Check if the IFO and BUP file contents are exactly the same
		free(buffer);
		close(streamout_ifo);
		close(streamout_bup);

		ifoClose(ifo);
		ifo = NULL;

	}

	if(ifo)
		ifoClose(ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	return 0;

}
