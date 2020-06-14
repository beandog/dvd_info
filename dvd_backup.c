#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#ifdef __linux__
#include <linux/cdrom.h>
#include <linux/limits.h>
#else
#include <limits.h>
#endif
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_config.h"
#include "dvd_device.h"
#include "dvd_open.h"
#include "dvd_specs.h"
#include "dvd_info.h"
#include "dvd_vmg_ifo.h"
#include "dvd_vts.h"
#include "dvd_vob.h"

	/**
	 *
	 *       _          _     _                _
	 *    __| |_   ____| |   | |__   __ _  ___| | ___   _ _ __
	 *   / _` \ \ / / _` |   | '_ \ / _` |/ __| |/ / | | | '_ \
	 *  | (_| |\ V / (_| |   | |_) | (_| | (__|   <| |_| | |_) |
	 *   \__,_| \_/ \__,_|___|_.__/ \__,_|\___|_|\_\\__,_| .__/
	 *                  |_____|                          |_|
	 *
	 * ** back up the DVD IFO, BUP, VTS and VOBs **
	 *
	 * dvd_backup is a tiny little program to clone the DVD as much as possible. The IFO and BUP
	 * files on a DVD store the metadata, while VOBs store the menus and the audio / video.
	 *
	 */

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

int main(int, char **);
int dvd_block_rw(dvd_file_t *, uint64_t, int);

int dvd_block_rw(dvd_file_t *dvdread_vts_file, uint64_t offset, int fd) {

	ssize_t rw = 0;
	unsigned char buffer[2048];

	rw = DVDReadBlocks(dvdread_vts_file, (size_t)offset, 1, buffer);

	if(rw < 0) {
		memset(buffer, '\0', DVD_VIDEO_LB_LEN);
		rw = write(fd, buffer, DVD_VIDEO_LB_LEN);
		return 1;
	}

	rw = write(fd, buffer, DVD_VIDEO_LB_LEN);

	if(rw < 0)
		return 2;

	return 0;

}

int main(int argc, char **argv) {

	const char *device_filename;
	int retval = 0;

	struct option p_long_opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "vts", required_argument, NULL, 'T' },
		{ "version", no_argument, NULL, 'V' },
		{ 0, 0, 0, 0 },
	};

	int opt = 0;
	int ix = 0;
	opterr = 1;

	bool opt_vts_number = false;
	uint16_t arg_vts_number = 0;

	while((opt = getopt_long(argc, argv, "hT:V", p_long_opts, &ix)) != -1) {

		switch(opt) {

			case 'h':
				printf("dvd_backup - backup a DVD\n");
				printf("\n");
				printf("Usage: dvd_backup [path] [options]\n");
				printf("\n");
				printf("Options:\n");
				printf("  -T, --vts <number>    Back up video title set number (default: all)\n");
				printf("\n");
				printf("DVD path can be a device name, a single file, or a directory (default: %s)\n", DEFAULT_DVD_DEVICE);
				return 0;
				break;

			case 'T':
				opt_vts_number = true;
				arg_vts_number = (uint16_t)strtoumax(optarg, NULL, 0);
				if(arg_vts_number < 1 || arg_vts_number > 99) {
					printf("VTS must be between 1 and 99\n");
					return 1;
				}
				break;

			case 'V':
				printf("dvd_backup %s\n", PACKAGE_VERSION);
				return 0;
				break;

			case 0:
			default:
				break;

		}

	}

	if (argv[optind])
		device_filename = argv[optind];
	else
		device_filename = DEFAULT_DVD_DEVICE;

	retval = device_open(device_filename);
	if(retval == 1)
		return 1;

	dvd_reader_t *dvdread_dvd = NULL;
	dvdread_dvd = dvdread_open(device_filename);

	if(dvdread_dvd == NULL)
		return 1;

	printf("* Opening VMG IFO\n");
	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	// Open VMG IFO -- where all the cool stuff is
	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "Could not open VMG IFO\n");
		return 1;
	}

	uint16_t num_ifos;
	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	printf("* %d Video Title Sets present\n", num_ifos);

	if(num_ifos < 1) {
		printf("* DVD has no title IFOs?!\n");
		return 1;
	}

	// Get the disc title
	char backup_title[DVD_TITLE + 1];
	memset(backup_title, '\0', DVD_TITLE + 1);
	dvd_title(backup_title, device_filename);
	if(strlen(backup_title) == 0) {
		strcpy(backup_title, "DVD_VIDEO");
	}

	// Build the backup directory
	char dvd_backup_dir[PATH_MAX];
	memset(dvd_backup_dir, '\0', PATH_MAX);
	snprintf(dvd_backup_dir, PATH_MAX, "%s", backup_title);

	// Use name first
	retval = mkdir(dvd_backup_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(retval == -1 && errno != EEXIST) {
		printf("* could not create backup directory: %s\n", dvd_backup_dir);
		return 1;
	}

	// And VIDEO_TS sub-directory second
	snprintf(dvd_backup_dir, PATH_MAX, "%s/VIDEO_TS", backup_title);
	retval = mkdir(dvd_backup_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(retval == -1 && errno != EEXIST) {
		printf("* could not create backup directory: %s\n", dvd_backup_dir);
		return 1;
	}

	/**
	 * Backup the .IFO and .BUP info files first, followed by the menu VOBS, and then finally
	 * the actual title set VOBs. The reason being that the IFO and BUPs are most likely to
	 * able to be read and copied, the menus as well because of their small size, and the
	 * video title sets will cause the most problems, if any.
	 */

	// Keep track of each backup in one struct
	ssize_t dvd_backup_blocks = 0;
	char dvd_backup_filename[PATH_MAX];
	memset(dvd_backup_filename, '\0', PATH_MAX);

	// Start with the primary IFO first, and the backup BUP second
	bool info_file = true;
	bool ifo_backed_up = false;
	bool bup_backed_up = false;

	ssize_t ifo_bytes_read = 0;
	ssize_t ifo_bytes_written = 0;

	ssize_t dvd_block = 0;
	dvd_file_t *dvdread_ifo_file = NULL;

	// Loop through all the IFOs and copy the .IFO and .BUP files
	uint16_t ifo_number = 0;
	uint8_t ifo_buffer[DVD_VIDEO_LB_LEN];
	memset(ifo_buffer, '\0', DVD_VIDEO_LB_LEN);
	char vts_filename[23]; // Example string: VIDEO_TS/VIDEO_TS.VOB
	memset(vts_filename, '\0', 23);
	ifo_handle_t *ifo = NULL;
	int ifo_fd = -1;
	for (ifo_number = 0; ifo_number < num_ifos + 1; ifo_number++) {

		// Always write the VMG IFO, and skip others if optional one is passed
		if(ifo_number && opt_vts_number && arg_vts_number != ifo_number)
			continue;

		info_file = true;
		ifo_backed_up = false;
		bup_backed_up = false;

		ifo = ifoOpen(dvdread_dvd, ifo_number);

		// TODO work around broken IFOs by copying contents directly to filesystem
		if(ifo == NULL) {
			// printf("* Opening IFO FAILED\n");
			// printf("* Skipping IFO\n");
			continue;
		}

		// Loop to backup both
		while(ifo_backed_up == false || bup_backed_up == false) {

			// The .IFO is on the inside of the optical disc, while the .BUP is on the outside.
			dvdread_ifo_file = DVDOpenFile(dvdread_dvd, ifo_number, info_file ? DVD_READ_INFO_FILE : DVD_READ_INFO_BACKUP_FILE);

			if(dvdread_ifo_file == 0) {
				// printf("* Opening IFO FAILED\n");
				// printf("* Skipping IFO\n");
				ifoClose(ifo);
				ifo = NULL;
				continue;
			}

			// Get the number of blocks plus filesize
			dvd_backup_blocks = DVDFileSize(dvdread_ifo_file);

			if(dvd_backup_blocks < 0) {
				// printf("* Could not determine IFO filesize, skipping\n");
				ifoClose(ifo);
				ifo = NULL;
				continue;
			}

			// Seek to beginning of file
			DVDFileSeek(ifo->file, 0);

			if(ifo_number == 0) {
				sprintf(vts_filename, "VIDEO_TS.%s", info_file ? "IFO" : "BUP");
				snprintf(dvd_backup_filename, PATH_MAX, "%s/%s", dvd_backup_dir, vts_filename);
			} else {
				sprintf(vts_filename, "VTS_%02" PRIu16 "_0.%s", ifo_number, info_file ? "IFO" : "BUP");
				snprintf(dvd_backup_filename, PATH_MAX, "%s/%s", dvd_backup_dir, vts_filename);
			}

			// file handlers

			if(access(dvd_backup_filename, F_OK) != 0) {

				printf("* Writing to %s\n", vts_filename);
				ifo_fd = open(dvd_backup_filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
				if(ifo_fd == -1) {
					printf("* Could not create %s\n", vts_filename);
					return 1;
				}

				// Read / write IFO or BUP
				memset(ifo_buffer, '\0', DVD_VIDEO_LB_LEN);
				dvd_block = 0;
				ifo_bytes_read = 0;
				ifo_bytes_written = 0;

				// In the case of IFOs and BUPs, be pedantic and read only one block at a time plus
				// always count one as written
				while(dvd_block < dvd_backup_blocks) {
					ifo_bytes_read = DVDReadBytes(ifo->file, ifo_buffer, DVD_VIDEO_LB_LEN);
					if(ifo_bytes_read < 0)
						memset(ifo_buffer, '\0', DVD_VIDEO_LB_LEN);
					ifo_bytes_written = write(ifo_fd, ifo_buffer, DVD_VIDEO_LB_LEN);
					dvd_block++;
				}

				retval = close(ifo_fd);

			}

			// Switch to next one
			if(info_file) {
				ifo_backed_up = true;
				info_file = false;
			} else {
				bup_backed_up = true;
			}

		}

		ifoClose(ifo);
		ifo = NULL;

	}

	/** VOB copy variables **/
	uint64_t vob_block = 0;

	/** copy title sets **/
	struct dvd_info dvd_info;
	dvd_info = dvd_info_open(dvdread_dvd, device_filename);
	if(dvd_info.valid == 0)
		return 1;

	uint16_t vts = 1;
	bool has_invalid_ifos = false;
	struct dvd_vts dvd_vts[99];

	// Exit if all the IFOs cannot be opened
	ifo_handle_t *vts_ifos[DVD_MAX_VTS_IFOS];
	vts_ifos[0] = NULL;

	uint16_t vob = 0;

	// Scan VTS for invalid data
	for(vts = 0; vts < dvd_info.video_title_sets + 1; vts++) {

		dvd_vts[vts].vts = vts;
		dvd_vts[vts].valid = false;
		dvd_vts[vts].blocks = 0;
		dvd_vts[vts].filesize = 0;
		dvd_vts[vts].vobs = 0;

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(vts_ifos[vts] == NULL) {
			dvd_vts[vts].valid = false;
			has_invalid_ifos = true;
			vts_ifos[vts] = NULL;
			continue;
		}

		if(!ifo_is_vmg(vts_ifos[vts]) && !ifo_is_vts(vts_ifos[vts])) {
			ifoClose(vts_ifos[vts]);
			vts_ifos[vts] = NULL;
			continue;
		}

		dvd_vts[vts].valid = true;
		dvd_vts[vts].blocks = dvd_vts_blocks(dvdread_dvd, vts);
		dvd_vts[vts].filesize = dvd_vts_filesize(dvdread_dvd, vts);
		dvd_vts[vts].vobs = dvd_vts_vobs(dvdread_dvd, vts);

		/*
		printf("* Blocks: %zu\n", dvd_vts[vts].blocks);
		printf("* Filesize: %" PRIu64 "\n", dvd_vts[vts].filesize);
		printf("* VOBs: %u\n", dvd_vts[vts].vobs);
		*/

		for(vob = 0; vob < dvd_vts[vts].vobs + 1; vob++) {
			dvd_vts[vts].dvd_vobs[vob].vts = vts;
			dvd_vts[vts].dvd_vobs[vob].vob = vob;
			dvd_vts[vts].dvd_vobs[vob].filesize = dvd_vob_filesize(dvdread_dvd, vts, vob);
			dvd_vts[vts].dvd_vobs[vob].blocks = dvd_vob_blocks(dvdread_dvd, vts, vob);
		}

	}

	// Create blank placeholder VOBs which will be populated
	// Also copy just the menu title sets for now. On broken DVDs, the
	// title VOBs could be garbage, so they will be copied later.
	int vob_fd = -1;
	char vob_filename[PATH_MAX];
	memset(vob_filename, '\0', PATH_MAX);

	/** Backup VIDEO_TS.VOB **/
	snprintf(vob_filename, PATH_MAX, "%s/VIDEO_TS.VOB", dvd_backup_dir);

	uint64_t dvd_blocks_offset = 0;
	uint64_t dvd_blocks_skipped = 0;

	struct stat vob_stat;

	// Copy the menu title vobs
	/** Backup VIDEO_TS.VOB, VTS_01_0.VOB to VTS_99_0.VOB **/
	dvd_file_t *dvdread_vts_file = NULL;
	for(vts = 0; vts < dvd_info.video_title_sets + 1; vts++) {

		// If passed the --vts argument, skip if not this one
		if(opt_vts_number && arg_vts_number != vts)
			continue;

		// Skip if the file doesn't exist on the DVD
		dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_MENU_VOBS);
		if(dvdread_vts_file == 0)
			continue;

		if(vts == 0)
			snprintf(vob_filename, PATH_MAX, "%s/VIDEO_TS.VOB", dvd_backup_dir);
		else
			snprintf(vob_filename, PATH_MAX, "%s/VTS_%02" PRIu16  "_0.VOB", dvd_backup_dir, vts);

		// Skip if file exists
		if(access(vob_filename, F_OK) == 0) {
			retval = stat(vob_filename, &vob_stat);
			printf("source %s bytes: %" PRIu64 "\n", vob_filename, dvd_vts[vts].dvd_vobs[0].blocks * DVD_VIDEO_LB_LEN);
			printf("target %s bytes: %li\n", vob_filename, vob_stat.st_size);
			continue;
		}

		vob_fd = open(vob_filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);

		// It's not unusual for a DVD to have a placeholder VOB, if
		// that's the case, skip it after it's been created by open().
		if(dvd_vts[vts].dvd_vobs[0].blocks == 0) {
			close(vob_fd);
			continue;
		}

		dvd_blocks_offset = 0;
		dvd_blocks_skipped = 0;

		while(dvd_blocks_offset < dvd_vts[vts].dvd_vobs[0].blocks) {

			retval = dvd_block_rw(dvdread_vts_file, dvd_blocks_offset, vob_fd);

			// Skipped a block
			if(retval == 1)
				dvd_blocks_skipped++;

			// Couldn't write
			if(retval == 2) {
				fprintf(stdout, "* couldn't write to %s\n", vob_filename);
				fflush(stdout);
				return 1;
			}

			fprintf(stdout, "* %s blocks written: %" PRIu64 " of %" PRIu64 "\r", vob_filename, dvd_blocks_offset + 1, dvd_vts[vts].dvd_vobs[0].blocks);
			fflush(stdout);

			dvd_blocks_offset++;

		}

		printf("\n");

		close(vob_fd);

		vob_fd = -1;

		DVDCloseFile(dvdread_vts_file);

	}

	/** Backup VTS_01_1.VOB through VTS_99_9.VOB **/
	ssize_t vob_blocks_skipped = 0;
	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		if(opt_vts_number && arg_vts_number != vts)
			continue;

		if(dvd_vts[vts].valid == false)
			continue;

		dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_TITLE_VOBS);

		if(dvdread_vts_file == 0)
			continue;

		printf("[VTS %d]\n", vts);

		printf("* Blocks: %" PRIu64 "\n", dvd_vts[vts].blocks);
		printf("* Filesize: %zu\n", dvd_vts[vts].filesize);
		printf("* VOBs: %u\n", dvd_vts[vts].vobs);

		for(vob = 1; vob < dvd_vts[vts].vobs + 1; vob++)
			if(dvd_vts[vts].dvd_vobs[vob].blocks)
				printf("* VOB %i filesize: %zu\n", vob, dvd_vob_filesize(dvdread_dvd, vts, vob));

		dvd_blocks_offset = 0;

		for(vob = 1; vob < dvd_vts[vts].vobs + 1; vob++) {

			snprintf(vob_filename, PATH_MAX, "%s/VTS_%02" PRIu16 "_%" PRIu16 ".VOB", dvd_backup_dir, vts, vob);

			// Skip existing file and increase block offset
			if(access(vob_filename, F_OK) == 0) {
				dvd_blocks_offset += dvd_vts[vts].dvd_vobs[vob].blocks;
				continue;
			}

			vob_fd = open(vob_filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);

			vob_block = 0;
			vob_blocks_skipped = 0;

			while(vob_block < dvd_vts[vts].dvd_vobs[vob].blocks) {

				retval = dvd_block_rw(dvdread_vts_file, dvd_blocks_offset, vob_fd);

				// Skipped a block
				if(retval == 1)
					vob_blocks_skipped++;

				// Couldn't write
				if(retval == 2) {
					printf("* couldn't write to %s\n", vob_filename);
					return 1;
				}

				fprintf(stdout, "* %s blocks written: %" PRIu64 " of %" PRIu64 ", skipped: %" PRIu64 "\r", vob_filename, vob_block + 1, dvd_vts[vts].dvd_vobs[vob].blocks, vob_blocks_skipped);
				fflush(stdout);

				dvd_blocks_offset++;
				vob_block++;

			}

			close(vob_fd);

			vob_fd = -1;

			printf("\n");

		}

		DVDCloseFile(dvdread_vts_file);

	}

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	return 0;

}
