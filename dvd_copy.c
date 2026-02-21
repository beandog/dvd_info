#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#ifdef __linux__
#include <linux/cdrom.h>
#include <linux/limits.h>
#include "dvd_drive.h"
#else
#include <limits.h>
#endif
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_config.h"
#include "dvd_info.h"
#include "dvd_open.h"
#include "dvd_device.h"
#include "dvd_drive.h"
#include "dvd_vmg_ifo.h"
#include "dvd_track.h"
#include "dvd_chapter.h"
#include "dvd_cell.h"
#include "dvd_video.h"
#include "dvd_audio.h"
#include "dvd_subtitles.h"
#include "dvd_time.h"

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

	/**
	 *      _          _
	 *   __| |_   ____| |    ___ ___  _ __  _   _
	 *  / _` \ \ / / _` |   / __/ _ \| '_ \| | | |
	 * | (_| |\ V / (_| |  | (_| (_) | |_) | |_| |
	 *  \__,_| \_/ \__,_|___\___\___/| .__/ \__, |
	 *                 |_____|       |_|    |___/
	 *
	 * ** copy a DVD track to the filesystem **
	 *
	 */

int main(int, char **);
void dvd_track_info(struct dvd_track *dvd_track, uint16_t track_number, ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo);

struct dvd_copy {
	uint16_t track;
	uint8_t first_chapter;
	uint8_t last_chapter;
	uint8_t first_cell;
	uint8_t last_cell;
	uint64_t blocks;
	uint64_t filesize;
	double filesize_mbs;
	char filename[PATH_MAX];
	int fd;
	unsigned char buffer[DVD_VIDEO_LB_LEN];
};

int main(int argc, char **argv) {

	char device_filename[PATH_MAX];
	bool opt_track_number = false;
	bool opt_chapter_number = false;
	bool opt_cell_number = false;
	bool p_dvd_copy = true;
	bool p_dvd_cat = false;
	bool opt_filename = false;
	uint16_t arg_track_number = 1;
	int long_index = 0;
	int opt = 0;
	bool invalid_opt = false;
	unsigned long int arg_number = 0;
	uint8_t arg_first_chapter = 1;
	uint8_t arg_last_chapter = 99;
	uint8_t arg_first_cell = 1;
	uint8_t arg_last_cell = 99;
	char *token = NULL;
	struct dvd_copy dvd_copy;

	struct option long_options[] = {

		{ "chapter", required_argument, 0, 'c' },
		{ "cells", required_argument, 0, 'd' },
		{ "dvd_copy.filename", required_argument, 0, 'o' },
		{ "track", required_argument, 0, 't' },
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },
		{ 0, 0, 0, 0 }

	};

	dvd_copy.track = 1;
	dvd_copy.first_chapter = 1;
	dvd_copy.last_chapter = 99;
	dvd_copy.first_cell = 1;
	dvd_copy.last_cell = 1;
	dvd_copy.blocks = 0;
	dvd_copy.filesize = 0;
	dvd_copy.filesize_mbs = 0;
	dvd_copy.fd = -1;
	memset(dvd_copy.buffer, '\0', DVD_VIDEO_LB_LEN);
	memset(dvd_copy.filename, '\0', PATH_MAX);

	while((opt = getopt_long(argc, argv, "c:d:ho:t:V", long_options, &long_index )) != -1) {

		switch(opt) {

			case 'c':
				opt_chapter_number = true;
				token = strtok(optarg, "-");
				if(strlen(token) > 2) {
					fprintf(stderr, "[dvd_copy] Chapter range must be between 1 and 99\n");
					return 1;
				}
				arg_number = strtoul(token, NULL, 10);
				if(arg_number > 99)
					arg_first_chapter = 99;
				else if(arg_number == 0)
					arg_first_chapter = 1;
				else
					arg_first_chapter = (uint8_t)arg_number;

				token = strtok(NULL, "-");

				if(token == NULL) {
					arg_last_chapter = arg_first_chapter;
				}

				if(token != NULL) {
					if(strlen(token) > 2) {
						fprintf(stderr, "[dvd_copy] Chapter range must be between 1 and 99\n");
						return 1;
					}
					arg_number = strtoul(token, NULL, 10);
					if(arg_number > 99)
						arg_last_chapter = 99;
					if(arg_number == 0)
						arg_last_chapter = arg_first_chapter;
					else
						arg_last_chapter = (uint8_t)arg_number;
				}

				if(arg_last_chapter < arg_first_chapter)
					arg_last_chapter = arg_first_chapter;

				if(arg_first_chapter > arg_last_chapter)
					arg_first_chapter = arg_last_chapter;
				break;

			case 'd':
				opt_cell_number = true;
				token = strtok(optarg, "-"); {
					if(strlen(token) > 2) {
						fprintf(stderr, "[dvd_copy] Cell range must be between 1 and 99\n");
						return 1;
					}
					arg_number = strtoul(token, NULL, 10);
					if(arg_number > 99)
						arg_first_cell = 99;
					else if(arg_number == 0)
						arg_first_cell = 1;
					else
						arg_first_cell = (uint8_t)arg_number;
				}

				token = strtok(NULL, "-");

				if(token == NULL) {
					arg_last_cell = arg_first_cell;
				}

				if(token != NULL) {
					if(strlen(token) > 2) {
						fprintf(stderr, "[dvd_copy] Cell range must be between 1 and 99\n");
						return 1;
					}
					arg_number = strtoul(token, NULL, 10);
					if(arg_number > 99)
						arg_last_cell = 99;
					else if(arg_number == 0)
						arg_last_cell = arg_first_cell;
					else
						arg_last_cell = (uint8_t)arg_number;
				}

				if(arg_last_cell < arg_first_cell)
					arg_last_cell = arg_first_cell;
				if(arg_first_cell > arg_last_cell)
					arg_first_cell = arg_last_cell;

				break;

			case 'o':
				if(strlen(optarg) == 1 && strncmp("-", optarg, 1) == 0) {
					p_dvd_copy = false;
					p_dvd_cat = true;
				} else {
					p_dvd_copy = true;
					p_dvd_cat = false;
					opt_filename = true;
					strncpy(dvd_copy.filename, optarg, PATH_MAX - 1);
				}
				break;

			case 't':
				opt_track_number = true;
				arg_number = strtoul(optarg, NULL, 10);
				if(arg_number > 99)
					arg_track_number = 99;
				else if(arg_number == 0)
					arg_track_number = 1;
				else
					arg_track_number = (uint16_t)arg_number;
				break;

			case 'V':
				printf("dvd_copy %s\n", PACKAGE_VERSION);
				return 0;

			// ignore unknown arguments
			case '?':
				invalid_opt = true;
			case 'h':
				printf("dvd_copy - copy a single DVD track\n");
				printf("\n");
				printf("Usage: dvd_copy [path] [options]\n");
				printf("\n");
				printf("Options:\n");
				printf("  -t, --track <number>     Copy selected track (default: longest)\n");
				printf("  -c, --chapter <#>[-#]    Copy chapter number or range (default: all)\n");
				printf("  -o, --output <filename>  Save to filename (default: dvd_track_##.mpg)\n");
				printf("      --output -           Write to stdout\n");
				printf("\n");
				printf("DVD path can be a device name, a single file, or directory (default: %s)\n", DEFAULT_DVD_DEVICE);
				if(invalid_opt)
					return 1;
				return 0;

			// let getopt_long set the variable
			case 0:
			default:
				break;

		}

	}

	// Setting a cell range requires a chapter to be selected; If none is specified, use the first chapter only
	if(opt_cell_number && !opt_chapter_number) {
		opt_chapter_number = true;
		arg_first_chapter = 1;
		arg_last_chapter = 1;
	}

	memset(device_filename, '\0', PATH_MAX);
	if (argv[optind])
		strncpy(device_filename, argv[optind], PATH_MAX - 1);
	else
		strncpy(device_filename, DEFAULT_DVD_DEVICE, PATH_MAX - 1);

	/** Begin dvd_copy :) */

	dvd_reader_t *dvdread_dvd = NULL;
	dvd_logger_cb dvdread_logger_cb = { dvd_info_logger_cb };

	// Open the DVD with libdvdread
	dvdread_dvd = DVDOpen2(NULL, &dvdread_logger_cb, device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "Opening DVD with dvdread %s failed\n", device_filename);
		return 1;
	}

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || vmg_ifo->vts_atrt == NULL) {
		fprintf(stderr, "[dvd_copy] Could not open VMG IFO\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// DVD
	struct dvd_info dvd_info;
	dvd_info = dvd_info_open(dvdread_dvd, device_filename);
	if(dvd_info.valid == 0)
		return 1;
	if(p_dvd_copy)
		printf("Disc title: %s\n", dvd_info.title);


	uint16_t num_ifos = 1;
	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	if(num_ifos < 1) {
		fprintf(stderr, "[dvd_copy] DVD has no title IFOs\n");
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Track
	struct dvd_track dvd_track;
	memset(&dvd_track, 0, sizeof(dvd_track));

	struct dvd_track dvd_tracks[DVD_MAX_TRACKS];
	memset(&dvd_tracks, 0, sizeof(dvd_track) * dvd_info.tracks);

	// Cells
	struct dvd_cell dvd_cell;
	dvd_cell.cell = 1;
	memset(dvd_cell.length, '\0', sizeof(dvd_cell.length));
	snprintf(dvd_cell.length, DVD_CELL_LENGTH + 1, "00:00:00.000");
	dvd_cell.msecs = 0;

	// Open first IFO
	uint16_t vts = 1;
	ifo_handle_t *vts_ifo = NULL;

	vts_ifo = ifoOpen(dvdread_dvd, vts);
	if(vts_ifo == NULL) {
		fprintf(stderr, "[dvd_copy] Could not open VTS IFO for track %" PRIu16 "\n", 1);
		return 1;
	}
	ifoClose(vts_ifo);
	vts_ifo = NULL;

	// Create an array of all the IFOs
	ifo_handle_t *vts_ifos[DVD_MAX_VTS_IFOS];
	vts_ifos[0] = NULL;

	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(!vts_ifos[vts]) {
			vts_ifos[vts] = NULL;
		} else if(!ifo_is_vts(vts_ifos[vts])) {
			ifoClose(vts_ifos[vts]);
			vts_ifos[vts] = NULL;
		}

	}

	// Exit if track number requested does not exist
	if(opt_track_number && (arg_track_number > dvd_info.tracks)) {
		fprintf(stderr, "[dvd_copy] Invalid track number %" PRIu16 "\n", arg_track_number);
		fprintf(stderr, "[dvd_copy] Valid track numbers: 1 to %" PRIu16 "\n", dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	} else if(opt_track_number) {
		dvd_copy.track = arg_track_number;
	}

	uint16_t ix = 0;
	uint16_t track = 1;

	uint32_t longest_msecs = 0;

	for(ix = 0, track = 1; ix < dvd_info.tracks; ix++, track++) {

		vts = dvd_vts_ifo_number(vmg_ifo, track);
		vts_ifo = vts_ifos[vts];
		dvd_track_info(&dvd_tracks[ix], track, vmg_ifo, vts_ifo);

		if(dvd_tracks[ix].msecs > longest_msecs) {
			dvd_info.longest_track = track;
			longest_msecs = dvd_tracks[ix].msecs;
		}

	}

	// Set the track number to rip if none is passed as an argument
	if(!opt_track_number)
		dvd_copy.track = dvd_info.longest_track;

	dvd_track = dvd_tracks[dvd_copy.track - 1];

	// Set the proper chapter range
	if(opt_chapter_number) {
		if(arg_first_chapter > dvd_track.chapters) {
			dvd_copy.first_chapter = dvd_track.chapters;
			fprintf(stderr, "[dvd_copy] Resetting first chapter to %" PRIu8 "\n", dvd_copy.first_chapter);
		} else
			dvd_copy.first_chapter = arg_first_chapter;

		if(arg_last_chapter > dvd_track.chapters) {
			dvd_copy.last_chapter = dvd_track.chapters;
			fprintf(stderr, "[dvd_copy] Resetting last chapter to %" PRIu8 "\n", dvd_copy.last_chapter);
		} else
			dvd_copy.last_chapter = arg_last_chapter;
	} else {
		dvd_copy.first_chapter = 1;
		dvd_copy.last_chapter = dvd_track.chapters;
	}

	// Set the proper cell range
	if(opt_cell_number) {
		if(arg_first_cell > dvd_track.cells) {
			dvd_copy.first_cell = dvd_track.cells;
			fprintf(stderr, "[dvd_copy] Resetting first cell to %" PRIu8 "\n", dvd_copy.first_cell);
		} else
			dvd_copy.first_cell = arg_first_cell;

		if(arg_last_cell > dvd_track.cells) {
			dvd_copy.last_cell = dvd_track.cells;
			fprintf(stderr, "[dvd_copy] Resetting last cell to %" PRIu8 "\n", dvd_copy.last_cell);
		} else
			dvd_copy.last_cell = arg_last_cell;
	} else {
		dvd_copy.first_cell = 1;
		dvd_copy.last_cell = dvd_track.cells;
	}

	// Set default filename
	if(!opt_filename) {
		sprintf(dvd_copy.filename, "dvd_track_%02" PRIu16 ".mpg", dvd_copy.track);
	}

	/**
	 * File descriptors and filenames
	 */
	dvd_file_t *dvdread_vts_file = NULL;

	vts = dvd_vts_ifo_number(vmg_ifo, dvd_copy.track);
	vts_ifo = vts_ifos[vts];

	// Open the VTS VOB
	dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_TITLE_VOBS);
	if(dvdread_vts_file == NULL) {
		fprintf(stderr, "Could not open VTS VOB %" PRIu16 "\n", vts);
		ifoClose(vts_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}

	if(p_dvd_copy)
		printf("Track: %*" PRIu16 ", Length: %s, Chapters: %*" PRIu8 ", Cells: %*" PRIu8 ", Audio streams: %*" PRIu8 ", Subpictures: %*" PRIu8 ", Title set: %*" PRIu16 ", Filesize: %.0lf MBs\n", 2, dvd_track.track, dvd_track.length, 2, dvd_track.chapters, 2, dvd_track.cells, 2, dvd_track.audio_tracks, 2, dvd_track.subtitles, 2, vts, dvd_track.filesize_mbs);

	// Check for track issues
	dvd_track.valid = true;

	if(dvd_track.msecs == 0) {
		printf("        Error: track has zero length\n");
		dvd_track.valid = false;
	}

	if(dvd_track.chapters == 0) {
		printf("        Error: track has zero chapters\n");
		dvd_track.valid = false;
	}

	if(dvd_track.cells == 0) {
		printf("        Error: track has zero cells\n");
		dvd_track.valid = false;
	}

	if(dvd_track.valid == false) {
		printf("Track has been marked as invalid, quitting\n");
		return 1;
	}

	if(p_dvd_copy) {
		dvd_copy.fd = open(dvd_copy.filename, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
		if(dvd_copy.fd == -1) {
			fprintf(stderr, "[dvd_copy] Couldn't create file %s\n", dvd_copy.filename);
			return 1;
		}
	} else if(p_dvd_cat) {
		dvd_copy.fd = 1;
	}

	struct dvd_chapter dvd_chapter;

	// Get limits of copy
	for(dvd_chapter.chapter = dvd_copy.first_chapter; dvd_chapter.chapter < dvd_copy.last_chapter + 1; dvd_chapter.chapter++) {

		dvd_chapter.first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, dvd_copy.track, dvd_chapter.chapter);
		dvd_chapter.last_cell = dvd_chapter_last_cell(vmg_ifo, vts_ifo, dvd_copy.track, dvd_chapter.chapter);

		if(opt_cell_number == false) {
			dvd_copy.first_cell = dvd_chapter.first_cell;
			dvd_copy.last_cell = dvd_chapter.last_cell;
		}

		for(dvd_cell.cell = dvd_copy.first_cell; dvd_cell.cell < dvd_copy.last_cell + 1; dvd_cell.cell++) {
			dvd_cell.blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_copy.blocks += dvd_cell.blocks;
			dvd_cell.filesize = dvd_cell_filesize(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_copy.filesize += dvd_cell.filesize;
		}

	}

	// Get total filesize of copy
	dvd_copy.filesize_mbs = ceil(dvd_copy.filesize / 1048576.0);

	/**
	 * Integers for numbers of blocks read, copied, counters
	 */
	uint64_t cell_sectors = 0;
	uint64_t cell_block = 0;
	ssize_t cell_blocks_read = 0;
	ssize_t total_blocks_read = 0;
	ssize_t bytes_written = 0;
	ssize_t total_bytes_written = 0;

	// Copying DVD track
	double mbs_written = 0;
	double percent_complete = 0;
	for(dvd_chapter.chapter = dvd_copy.first_chapter; dvd_chapter.chapter < dvd_copy.last_chapter + 1; dvd_chapter.chapter++) {

		// Use dvd_copy struct as the first and last cell
		dvd_chapter.first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, dvd_copy.track, dvd_chapter.chapter);
		dvd_chapter.last_cell = dvd_chapter_last_cell(vmg_ifo, vts_ifo, dvd_copy.track, dvd_chapter.chapter);
		dvd_chapter_length(dvd_chapter.length, vmg_ifo, vts_ifo, dvd_copy.track, dvd_chapter.chapter);

		if(opt_cell_number == false) {
			dvd_copy.first_cell = dvd_chapter.first_cell;
			dvd_copy.last_cell = dvd_chapter.last_cell;
		}

		// for(dvd_cell.cell = dvd_chapter.first_cell; dvd_cell.cell < dvd_chapter.last_cell + 1; dvd_cell.cell++) {
		for(dvd_cell.cell = dvd_copy.first_cell; dvd_cell.cell < dvd_copy.last_cell + 1; dvd_cell.cell++) {

			dvd_cell.blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.filesize = dvd_cell_filesize(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell_length(dvd_cell.length, vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			cell_sectors = dvd_cell.last_sector - dvd_cell.first_sector;

			if(p_dvd_copy)
				printf("        Chapter: %*" PRIu8 ", Cell: %*" PRIu8 ", Filesize: % 5.0lf MBs\n", 2, dvd_chapter.chapter, 2, dvd_cell.cell, ceil(dvd_cell.filesize / 1048576.0));

			if(dvd_cell.last_sector > dvd_cell.first_sector)
				cell_sectors++;

			cell_block = dvd_cell.first_sector;

			// This is where you would change the boundaries -- are you dumping to a track file (no boundaries) or a VOB (boundaries)
			while(cell_block < dvd_cell.last_sector + 1) {

				cell_blocks_read = DVDReadBlocks(dvdread_vts_file, (size_t)cell_block, 1, dvd_copy.buffer);
				total_blocks_read += cell_blocks_read;

				// Zero out cells that couldn't be read
				if(cell_blocks_read == -1)
					memset(dvd_copy.buffer, '\0', DVD_VIDEO_LB_LEN);

				bytes_written = write(dvd_copy.fd, dvd_copy.buffer, DVD_VIDEO_LB_LEN);
				total_bytes_written += bytes_written;
				mbs_written = ceil(total_bytes_written / 1048576.0);

				if(cell_block == dvd_cell.last_sector)
					percent_complete = 100;
				else {
					percent_complete = floor((mbs_written / dvd_copy.filesize_mbs) * 100.0);
					if(percent_complete == 0.0)
						percent_complete = 1.0;
					else if(percent_complete == 100.0)
						percent_complete = 99.0;
				}

				fprintf(stderr, "Progress: %.0lf/%.0lf MBs (%.0lf%%)\r", mbs_written, dvd_copy.filesize_mbs, percent_complete);

				fflush(stderr);

				cell_block++;

			}

		}

	}

	fprintf(stderr, "Progress: %.0lf/%.0lf MBs (100%%)\r", dvd_copy.filesize_mbs, dvd_copy.filesize_mbs);
	fflush(stderr);

	close(dvd_copy.fd);

	DVDCloseFile(dvdread_vts_file);

	fprintf(stderr, "\n");

	if(vts_ifo)
		ifoClose(vts_ifo);

	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	return 0;

}

void dvd_track_info(struct dvd_track *dvd_track, uint16_t track_number, ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo) {

	dvd_track->track = track_number;
	dvd_track->valid = true;
	dvd_track->vts = dvd_vts_ifo_number(vmg_ifo, track_number);
	dvd_track->ttn = dvd_track_ttn(vmg_ifo, track_number);
	dvd_track_length(dvd_track->length, vmg_ifo, vts_ifo, track_number);
	dvd_track->msecs = dvd_track_msecs(vmg_ifo, vts_ifo, track_number);
	dvd_track->chapters = dvd_track_chapters(vmg_ifo, vts_ifo, track_number);
	dvd_track->audio_tracks = dvd_track_audio_tracks(vts_ifo);
	dvd_track->subtitles = dvd_track_subtitles(vts_ifo);
	dvd_track->active_audio_streams = dvd_audio_active_tracks(vmg_ifo, vts_ifo, track_number);
	dvd_track->active_subs = dvd_track_active_subtitles(vmg_ifo, vts_ifo, track_number);
	dvd_track->cells = dvd_track_cells(vmg_ifo, vts_ifo, track_number);
	dvd_track->blocks = dvd_track_blocks(vmg_ifo, vts_ifo, track_number);
	dvd_track->filesize = dvd_track_filesize(vmg_ifo, vts_ifo, track_number);
	dvd_track->filesize_mbs = dvd_track_filesize_mbs(vmg_ifo, vts_ifo, track_number);

}
