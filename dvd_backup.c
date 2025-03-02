#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#ifdef __linux__
#include <linux/cdrom.h>
#include <linux/limits.h>
#else
#include <limits.h>
#endif
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "config.h"
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
	 * ** back up the DVD IFOs, BUPs, VTSs and VOBs **
	 *
	 * dvd_backup is a tiny little program to clone the DVD as much as possible. The IFO and BUP
	 * files on a DVD store the metadata, while VOBs store the menus and the audio / video.
	 *
	 */

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

#define DVD_DIR_PATH_MAX (PATH_MAX - strlen("/VIDEO_TS.IFO"))

int main(int, char **);
int dvd_block_rw(dvd_file_t *, uint64_t, int);
uint64_t blocks_to_mbs(double);
uint64_t percent_completed(uint64_t, uint64_t);

/**
 * Read and write to the backup file. If there is an error, quit, and
 * dvd_backup will skip the blocks.
 */
int dvd_block_rw(dvd_file_t *dvdread_vts_file, uint64_t offset, int fd) {

	ssize_t bytes_read = 0;
	unsigned char buffer[DVD_VIDEO_LB_LEN];

	bytes_read = DVDReadBlocks(dvdread_vts_file, (size_t)offset, 1, buffer);

	if(bytes_read < 0)
		return 1;

	ssize_t bytes_written = 0;
	bytes_written = write(fd, buffer, DVD_VIDEO_LB_LEN);

	if(bytes_written < 0)
		return 2;

	return 0;

}

uint64_t blocks_to_mbs(double blocks) {

	if(blocks < 1)
		return 0;

	uint64_t mbs;
	mbs = (blocks * 2048 / 1048576.0) + 1;

	return mbs;

}

uint64_t percent_completed(uint64_t src, uint64_t dest) {

	if(dest >= src)
		return 100;

	uint64_t percent = ((float)dest / (float)src) * 100;

	if(percent == 0)
		return 1;

	if(percent == 100)
		return 99;

	return percent;

}

int main(int argc, char **argv) {

	int retval = 0;
	char device_filename[PATH_MAX];

	struct option p_long_opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "name", required_argument, NULL, 'n' },
		{ "ifos", no_argument, NULL, 'i' },
		{ "vts", required_argument, NULL, 'T' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "version", no_argument, NULL, 'V' },
		{ 0, 0, 0, 0 },
	};

	int opt = 0;
	int ix = 0;
	opterr = 1;

	bool verbose = false;

	bool opt_title_sets = true;
	bool opt_vts_number = false;
	uint16_t arg_vts_number = 0;

	char dvd_custom_dir[PATH_MAX];
	memset(dvd_custom_dir, '\0', PATH_MAX);

	while((opt = getopt_long(argc, argv, "hin:T:Vv", p_long_opts, &ix)) != -1) {

		switch(opt) {

			case 'h':
				printf("dvd_backup - Create a DVD backup to filesystem\n");
				printf("\n");
				printf("Usage: dvd_backup [path] [options]\n");
				printf("\n");
				printf("Options:\n");
				printf("  -n, --name            Set DVD name\n");
				printf("  -i, --ifos            Back up only the IFO and BUP files\n");
				printf("  -T, --vts <number>    Back up video title set number (default: all)\n");
				printf("  -v, --verbose         Verbose output\n");
				printf("\n");
				printf("DVD path can be a device name, a single file, or a directory (default: %s)\n", DEFAULT_DVD_DEVICE);
				return 0;
				break;

			case 'n':
				strncpy(dvd_custom_dir, basename(optarg), DVD_DIR_PATH_MAX - 1);
				break;

			case 'i':
				opt_title_sets = false;
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

			case 'v':
				verbose = true;
				break;

			case 0:
			default:
				break;

		}

	}

	memset(device_filename, '\0', PATH_MAX);
	if (argv[optind])
		strncpy(device_filename, argv[optind], PATH_MAX - 1);
	else
		strncpy(device_filename, DEFAULT_DVD_DEVICE, PATH_MAX - 1);

	printf("[DVD]\n");
	if(verbose)
		printf("* Opening device %s\n", device_filename);

	dvd_reader_t *dvdread_dvd = NULL;
	dvd_logger_cb dvdread_logger_cb = { dvd_info_logger_cb };
	dvdread_dvd = DVDOpen2(NULL, &dvdread_logger_cb, device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "Opening DVD %s failed\n", device_filename);
		return 1;
	}

	struct dvd_info dvd_info;
	if(verbose)
		printf("* Opening VMG IFO\n");
	dvd_info = dvd_info_open(dvdread_dvd, device_filename);
	if(dvd_info.valid == 0)
		return 1;

	printf("* %" PRIu16 " Video Title Sets\n", dvd_info.video_title_sets);

	if(dvd_info.video_title_sets < 1) {
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

	// Set backup directory to uppercase
	size_t l = strlen(backup_title);
	for(l = 0; l < strlen(backup_title); l++) {
		if(isalpha(backup_title[l]))
			backup_title[l] = toupper(backup_title[l]);
	}

	// Build the backup directory
	char dvd_parent_dir[PATH_MAX];
	char dvd_backup_dir[PATH_MAX];
	memset(dvd_parent_dir, '\0', PATH_MAX);
	memset(dvd_backup_dir, '\0', PATH_MAX);
	if(strlen(dvd_custom_dir)) {
		snprintf(dvd_parent_dir, DVD_DIR_PATH_MAX - 1, "%s/", dvd_custom_dir);
		snprintf(dvd_backup_dir, DVD_DIR_PATH_MAX - 1, "%s%s", dvd_parent_dir, "VIDEO_TS");
	} else {
		snprintf(dvd_parent_dir, DVD_DIR_PATH_MAX - 1, "%s/", backup_title);
		snprintf(dvd_backup_dir, DVD_DIR_PATH_MAX - 1, "%s%s", dvd_parent_dir, "VIDEO_TS");
	}

	// Use name first
#ifdef _WIN32
	retval = mkdir(dvd_parent_dir);
#else
	retval = mkdir(dvd_parent_dir, 0755);
#endif
	if(retval == -1 && errno != EEXIST) {
		printf("* could not create backup directory: %s\n", dvd_parent_dir);
		return 1;
	}

#ifdef _WIN32
	retval = mkdir(dvd_backup_dir);
#else
	retval = mkdir(dvd_backup_dir, 0755);
#endif
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
	for (ifo_number = 0; ifo_number < dvd_info.video_title_sets + 1; ifo_number++) {

		// Always write the VMG IFO, and skip others if optional one is passed
		if(ifo_number && opt_vts_number && arg_vts_number != ifo_number)
			continue;

		info_file = true;
		ifo_backed_up = false;
		bup_backed_up = false;

		ifo = ifoOpen(dvdread_dvd, ifo_number);

		// TODO work around broken IFOs by copying contents directly to filesystem
		if(ifo == NULL) {
			if(verbose) {
				printf("* Opening IFO FAILED\n");
				printf("* Skipping IFO\n");
			}
			continue;
		}

		// Loop to backup both
		while(ifo_backed_up == false || bup_backed_up == false) {

			// The .IFO is on the inside of the optical disc, while the .BUP is on the outside.
			dvdread_ifo_file = DVDOpenFile(dvdread_dvd, ifo_number, info_file ? DVD_READ_INFO_FILE : DVD_READ_INFO_BACKUP_FILE);

			if(dvdread_ifo_file == 0) {
				if(verbose) {
					printf("* Opening IFO FAILED\n");
					printf("* Skipping IFO\n");
				}
				ifoClose(ifo);
				ifo = NULL;
				continue;
			}

			// Get the number of blocks plus filesize
			dvd_backup_blocks = DVDFileSize(dvdread_ifo_file);

			if(dvd_backup_blocks < 0) {
				if(verbose)
					printf("* Could not determine IFO filesize, skipping\n");
				ifoClose(ifo);
				ifo = NULL;
				continue;
			}

			// Seek to beginning of file
			DVDFileSeek(dvdread_ifo_file, 0);

			if(ifo_number == 0) {
				sprintf(vts_filename, "VIDEO_TS.%s", info_file ? "IFO" : "BUP");
				snprintf(dvd_backup_filename, PATH_MAX - 1, "%s/%s", dvd_backup_dir, vts_filename);
			} else {
				sprintf(vts_filename, "VTS_%02" PRIu16 "_0.%s", ifo_number, info_file ? "IFO" : "BUP");
				snprintf(dvd_backup_filename, PATH_MAX - 1, "%s/%s", dvd_backup_dir, vts_filename);
			}

			if(verbose)
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
				ifo_bytes_read = DVDReadBytes(dvdread_ifo_file, ifo_buffer, DVD_VIDEO_LB_LEN);
				if(ifo_bytes_read < 0)
					memset(ifo_buffer, '\0', DVD_VIDEO_LB_LEN);
				ifo_bytes_written = write(ifo_fd, ifo_buffer, DVD_VIDEO_LB_LEN);
				if(ifo_bytes_written < 0)
					fprintf(stderr, "* %s could not write block %zd, skipping\n", dvd_backup_filename, dvd_block);
				dvd_block++;
			}

			retval = close(ifo_fd);

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

	// Exit if only writing IFO / BUP files
	if(opt_title_sets == false)
		return 0;

	/** VOB copy variables **/
	uint64_t vob_block = 0;

	uint16_t vts = 1;
	struct dvd_vts dvd_vts[99];

	// Exit if all the IFOs cannot be opened
	ifo_handle_t *vts_ifos[DVD_MAX_VTS_IFOS];
	vts_ifos[0] = NULL;

	uint16_t vob = 0;

	// Scan VTS for invalid data
	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		dvd_vts[vts].vts = vts;
		dvd_vts[vts].valid = false;
		dvd_vts[vts].blocks = 0;
		dvd_vts[vts].filesize = 0;
		dvd_vts[vts].vobs = 0;

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(vts_ifos[vts] == NULL) {
			dvd_vts[vts].valid = false;
			vts_ifos[vts] = NULL;
			continue;
		}

		if(!ifo_is_vts(vts_ifos[vts])) {
			ifoClose(vts_ifos[vts]);
			vts_ifos[vts] = NULL;
			continue;
		}

		dvd_vts[vts].valid = true;
		dvd_vts[vts].blocks = dvd_vts_blocks(dvdread_dvd, vts);
		dvd_vts[vts].filesize = dvd_vts_filesize(dvdread_dvd, vts);
		dvd_vts[vts].filesize_mbs = dvd_vts_filesize_mbs(dvdread_dvd, vts);
		dvd_vts[vts].vobs = dvd_vts_vobs(dvdread_dvd, vts);

		for(vob = 0; vob < dvd_vts[vts].vobs + 1; vob++) {
			dvd_vts[vts].dvd_vobs[vob].vts = vts;
			dvd_vts[vts].dvd_vobs[vob].vob = vob;
			dvd_vts[vts].dvd_vobs[vob].filesize = dvd_vob_filesize(dvdread_dvd, vts, vob);
			dvd_vts[vts].dvd_vobs[vob].filesize_mbs = dvd_vob_filesize_mbs(dvdread_dvd, vts, vob);
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
	snprintf(vob_filename, PATH_MAX - 1, "%s/VIDEO_TS.VOB", dvd_backup_dir);

	uint64_t dvd_blocks_offset = 0;
	uint64_t dvd_blocks_skipped = 0;

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
			snprintf(vob_filename, PATH_MAX - 1, "%s/VIDEO_TS.VOB", dvd_backup_dir);
		else
			snprintf(vob_filename, PATH_MAX - 1, "%s/VTS_%02" PRIu16  "_0.VOB", dvd_backup_dir, vts);

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

	// In megabytes:
	// 0 - total filesize of VTS
	// 1 - total filesize of source VOB being copied
	// 2 - total filesize of destination VOB backed up
	// 3 - percent of VOB backed up
	uint64_t filesize_mbs[4];

	/** Backup VTS_01_1.VOB through VTS_99_9.VOB **/
	ssize_t vob_blocks_skipped = 0;
	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		filesize_mbs[0] = 0;

		if(opt_vts_number && arg_vts_number != vts)
			continue;

		if(dvd_vts[vts].valid == false)
			continue;

		dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_TITLE_VOBS);

		if(dvdread_vts_file == 0)
			continue;

		printf("[VTS %" PRIu16"]\n", vts);

		for(vob = 1; vob < dvd_vts[vts].vobs + 1; vob++)
			filesize_mbs[0] += blocks_to_mbs(dvd_vts[vts].dvd_vobs[vob].blocks);

		printf("* %" PRIu16 " VOBs\n", dvd_vts[vts].vobs);
		printf("* Filesize: %" PRIu64 " MBs\n", filesize_mbs[0]);

		for(vob = 1; vob < dvd_vts[vts].vobs + 1; vob++)
			if(dvd_vts[vts].dvd_vobs[vob].blocks && verbose)
				printf("* VOB %" PRIu16 " filesize: %.0f MBs\n", vob, ceil(dvd_vob_filesize_mbs(dvdread_dvd, vts, vob)));
		dvd_blocks_offset = 0;

		for(vob = 1; vob < dvd_vts[vts].vobs + 1; vob++) {

			filesize_mbs[1] = blocks_to_mbs(dvd_vts[vts].dvd_vobs[vob].blocks);
			filesize_mbs[2] = 0;
			filesize_mbs[3] = 0;

			snprintf(vob_filename, PATH_MAX - 1, "%s/VTS_%02" PRIu16 "_%" PRIu16 ".VOB", dvd_backup_dir, vts, vob);

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

				filesize_mbs[2] = blocks_to_mbs(vob_block + 1);
				filesize_mbs[3] = percent_completed(filesize_mbs[1], filesize_mbs[2]);
				fprintf(stdout, "* %s: %" PRIu64 "/%" PRIu64 " MBs (%" PRIu64 "%%)\r", vob_filename, filesize_mbs[2], filesize_mbs[1], filesize_mbs[3]);
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
