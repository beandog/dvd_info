#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <inttypes.h>
#include <linux/cdrom.h>
#include <linux/limits.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_device.h"
#include "dvd_drive.h"

#define DEFAULT_DVD_DEVICE "/dev/dvd"

void print_usage(char *binary);

/**
 * Output on 'dvd_info -h'
 */
void print_usage(char *binary) {

	printf("Usage %s [options] [dvd path]\n", binary);
	printf("\n");

}


int main(int argc, char **argv) {

	int dvd_fd;
	int drive_status;
	uint16_t num_ifos = 1;
	uint16_t ifo_number = 0;
	bool is_hardware = false;
	bool verbose = false;
	char *device_filename = DEFAULT_DVD_DEVICE;
	dvd_reader_t *dvdread_dvd;
	ifo_handle_t *ifo;
	dvd_file_t *dvd_file;
	ssize_t dvd_filesize;
	unsigned char *buffer = NULL;
	ssize_t bytes_read;
	ssize_t ifo_bytes_written;
	ssize_t bup_bytes_written;
	int streamout_ifo = -1, streamout_bup = -1;
	char ifo_filename[PATH_MAX] = {'\0'};
	char bup_filename[PATH_MAX] = {'\0'};

	if (argc > 1) {
		device_filename = argv[1];
	}

	if(!dvd_device_access(device_filename)) {
		fprintf(stderr, "cannot access %s\n", device_filename);
		return 1;
	}

	dvd_fd = dvd_device_open(device_filename);
	if(dvd_fd < 0) {
		fprintf(stderr, "error opening %s\n", device_filename);
		return 1;
	}
	dvd_device_close(dvd_fd);

	// Poll drive status if it is hardware
	is_hardware = dvd_device_is_hardware(device_filename);
	if(is_hardware) {
		drive_status = dvd_drive_get_status(device_filename);
		// FIXME send to stderr
		if(verbose) {
			printf("drive status: ");
			dvd_drive_display_status(device_filename);
		}
	}

	if(is_hardware) {

		if(!dvd_drive_has_media(device_filename)) {

			// FIXME send to stderr
			printf("drive status: ");
			dvd_drive_display_status(device_filename);
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
	}

	num_ifos = ifo->vts_atrt->nr_of_vtss;

	printf("* %d IFOS present\n", num_ifos);

	if(num_ifos < 1) {
		printf("* DVD has no IFOS?!\n");
		ifoClose(ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}

	if(ifo)
		ifoClose(ifo);

	// Testing - Open IFO directly
	// See DVDCopyIfoBup() in dvdbackup.c for reference

	for (ifo_number = 0; ifo_number < num_ifos + 1; ifo_number++) {

		printf("[IFO]\n");
		printf("* Opening IFO %d\n", ifo_number);

		ifo = ifoOpen(dvdread_dvd, ifo_number);

		if(!ifo) {
			fprintf(stderr, "dvd_info: opening IFO %i failed\n", ifo_number);
			DVDClose(dvdread_dvd);
			return 1;
		}

		dvd_file = DVDOpenFile(dvdread_dvd, ifo_number, DVD_READ_INFO_FILE);

		if(dvd_file == 0) {
			printf("* Could not open IFO %d\n", ifo_number);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		dvd_filesize = DVDFileSize(dvd_file) * DVD_VIDEO_LB_LEN;
		printf("* Block filesize: %ld\n", dvd_filesize);

		// Allocate enough memory for the buffer, *now that we know the filesize*
		// printf("* Allocating buffer\n");
		buffer = (unsigned char *)malloc((unsigned long)dvd_filesize * sizeof(unsigned char));

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
		bytes_read = DVDReadBytes(ifo->file, buffer, (size_t)dvd_filesize);
		if(bytes_read != dvd_filesize) {
			printf(" * Bytes read and IFO filesize do not match: %ld, %ld\n", bytes_read, dvd_filesize);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}


		// TODO
		// * Create VIDEO_TS file if it does not exist
		// * Create IFO, BUP files if we have permission, etc.
		// See http://linux.die.net/man/3/stat
		// See http://linux.die.net/man/3/mkdir

		if(ifo_number == 0) {
			snprintf(ifo_filename, 18, "/tmp/VIDEO_TS.IFO");
			snprintf(bup_filename, 18, "/tmp/VIDEO_TS.BUP");
		} else {
			snprintf(ifo_filename, 18, "/tmp/VTS_%02i_0.IFO", ifo_number);
			snprintf(bup_filename, 18, "/tmp/VTS_%02i_0.BUP", ifo_number);
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
			printf("Could not open %s\n", bup_filename);
			free(buffer);
			close(streamout_ifo);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		printf("* Writing to %s\n", ifo_filename);
		ifo_bytes_written = write(streamout_ifo, buffer, (size_t)dvd_filesize);
		printf("* Writing to %s\n", bup_filename);
		bup_bytes_written = write(streamout_bup, buffer, (size_t)dvd_filesize);

		// Check that source size and target sizes match
		if((ifo_bytes_written != dvd_filesize) || (bup_bytes_written != dvd_filesize) || (ifo_bytes_written != bup_bytes_written)) {
			if(ifo_bytes_written != dvd_filesize)
				printf("* IFO num bytes written and IFO filesize do not match: %ld, %ld\n", ifo_bytes_written, dvd_filesize);
			if(bup_bytes_written != dvd_filesize)
				printf("* BUP num bytes written and BUP filesize do not match: %ld, %ld\n", bup_bytes_written, dvd_filesize);
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
