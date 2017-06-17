#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <linux/cdrom.h>
#include <linux/limits.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_device.h"
#include "dvd_vmg_ifo.h"
#include "dvd_vts.h"
#include "dvd_vob.h"
#include "dvd_track.h"
#include "dvd_chapter.h"
#include "dvd_cell.h"
#include "dvd_video.h"
#include "dvd_audio.h"
#include "dvd_subtitles.h"
#include "dvd_time.h"
#include "dvd_json.h"
#include "dvd_xchap.h"
#include "dvd_debug.h"
#ifdef __linux__
#include <linux/cdrom.h>
#include "dvd_drive.h"
#endif
#ifndef VERSION
#define VERSION "1.0"
#endif

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

#define DVD_COPY_BLOCK_LIMIT 512

// 2048 * 512 = 1 MB
#define DVD_COPY_BYTES_LIMIT ( DVD_COPY_BLOCK_LIMIT * DVD_VIDEO_LB_LEN )


void print_usage(char *binary) {

	printf("%s - Copy a DVD track\n", binary);
	printf("\n");
	printf("Usage: %s [-t track] [dvd path]\n", binary);
	printf("\n");
	printf("DVD path can be a directory, a device filename, or a local file.\n");
	printf("\n");
	printf("Examples:\n");
	printf("  dvd_info /dev/dvd	# Read a DVD drive directly\n");
	printf("  dvd_info movie.iso	# Read an image file\n");
	printf("  dvd_info movie/	# Read a directory that contains VIDEO_TS\n");
	printf("\n");
	printf("If no DVD path is given, %s is used in its place.\n", DEFAULT_DVD_DEVICE);

}

int main(int, char **);

ssize_t dvd_copy_blocks(const int fd, unsigned char *buffer, dvd_file_t *dvdread_vts_file, const int offset, const ssize_t num_blocks);

ssize_t dvd_copy_blocks(const int fd, unsigned char *buffer, dvd_file_t *dvdread_vts_file, const int offset, const ssize_t num_blocks) {

	// unsigned char *buffer = NULL;
	ssize_t num_copied = 0;
	ssize_t blocks = 0;

	buffer = (unsigned char *)calloc(1, (uint64_t)DVD_COPY_BYTES_LIMIT * sizeof(unsigned char));
	if(buffer == NULL)
		return -2;

	blocks = DVDReadBlocks(dvdread_vts_file, offset, (uint64_t)num_blocks, buffer);

	if(!blocks)
		return -1;

	if(blocks != num_blocks)
		return 0;
	
	num_copied = write(fd, buffer, (uint64_t)(blocks * DVD_VIDEO_LB_LEN));

	printf("wrote %lu num blocks offset: %i amount: %lu\n", num_copied, offset, num_blocks);

	return num_copied;

}

int main(int argc, char **argv) {

	int dvd_fd = 0;
	int vob_fd = 0;
	int track_fd = 0;
	// int cell_fd;
	int drive_status;
	uint16_t num_ifos = 1;
	// uint16_t ifo_number = 0;
	uint16_t track_number = 1;
	bool is_hardware = false;
	const char *device_filename = DEFAULT_DVD_DEVICE;
	dvd_reader_t *dvdread_dvd;
	uint16_t vts = 1;
	ifo_handle_t *ifo = NULL;
	ifo_handle_t *vmg_ifo;
	ifo_handle_t *vts_ifo = NULL;
	dvd_file_t *dvdread_ifo_file = NULL;
	dvd_file_t *dvdread_vts_file = NULL;
	dvd_file_t *dvdread_menu_file = NULL;
	ssize_t ifo_filesize;
	ssize_t menu_blocks;
	ssize_t menu_filesize;
	// uint8_t *buffer = NULL;
	unsigned char *dvd_read_buffer = NULL;
	uint32_t cell_sectors;
	ssize_t read_blocks;
	ssize_t dvd_read_blocks;
	int dvd_block_offset = 0;
	// ssize_t ifo_bytes_read;
	// ssize_t ifo_bytes_written;
	// ssize_t bup_bytes_written;
	ssize_t dvdread_read_blocks;
	ssize_t cell_blocks_written;
	// ssize_t vob_blocks_written;
	// ssize_t vts_blocks_written;
	ssize_t track_blocks_written;
	ssize_t bytes_written;
	// char ifo_filename[PATH_MAX] = {'\0'};
	// char bup_filename[PATH_MAX] = {'\0'};
	char track_filename[25] = {'\0'};
	// int z;

	bool copy_dvd = false;
	bool copy_vobs = false;
	bool copy_tracks = false;
	bool copy_cells = false;
	
	bool opt_track_number = false;
	int arg_track_number = 0;
	uint16_t d_first_track = 1;
	uint16_t d_last_track = 1;
	bool d_all_tracks = false;
	int long_index = 0;
	int opt = 0;
	// Send 'invalid argument' to stderr
	opterr= 1;
	// Check for invalid input
	// bool valid_args = true;
	const char *str_options;
	str_options = "ht:";

	struct option long_options[] = {

		{ "track", required_argument, 0, 't' },
		{ 0, 0, 0, 0 }

	};

	// parse options
	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {

		switch(opt) {

			case 'h':
				print_usage("dvd_copy");
				return 0;

			case 't':
				opt_track_number = true;
				arg_track_number = atoi(optarg);
				copy_tracks = true;
				break;

			// ignore unknown arguments
			case '?':
				print_usage("dvd_copy");
				return 1;

			// let getopt_long set the variable
			case 0:
			default:
				break;

		}

	}

	if(!copy_dvd && !copy_tracks) {
		print_usage("dvd_copy");
		return 1;
	}

	if (argv[optind])
		device_filename = argv[optind];
	else
		device_filename = DEFAULT_DVD_DEVICE;
	
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

	dvdread_dvd = DVDOpen(device_filename);

	if(!dvdread_dvd) {
		printf("* dvdread could not open %s\n", device_filename);
		return 1;
	}

	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL) {
		printf("* Could not open IFO zero\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	printf("Disc Title: %s\n", dvd_title(device_filename));

	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	if(num_ifos < 1) {
		printf("* DVD has no title IFOs?!\n");
		printf("* Most likely a bug in libdvdread or a bad master or problems reading the disc\n");
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Check if VIDEO_TS directory exists

	if(copy_dvd) {
		DIR *dir = opendir("VIDEO_TS");
		if(!dir) {
			int mkdir_retval;
			mkdir_retval = mkdir("VIDEO_TS", 0755);

			if(mkdir_retval == -1) {
				printf("Could not create directory VIDEO_TS\n");
				return 1;
			}
		}
	}

	// Open IFO directly
	// See DVDCopyIfoBup() in dvdbackup.c for reference
	/**
	for (ifo_number = 0; ifo_number < num_ifos + 1; ifo_number++) {

		ifo = ifoOpen(dvdread_dvd, ifo_number);

		// TODO work around broken IFOs by copying contents directly to filesystem
		if(ifo == NULL) {
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

		// Allocate enough memory for the buffer, *now that we know the filesize*
		buffer = (uint8_t *)calloc(1, (unsigned long)ifo_filesize * sizeof(uint8_t));

		if(buffer == NULL) {
			printf("* Could not allocate memory for buffer\n");
			ifoClose(ifo);
			ifo = NULL;
			DVDClose(dvdread_dvd);
			return 1;
		}

		// printf("* Seeking to beginning of file\n");
		DVDFileSeek(ifo->file, 0);

		// Need to check to make sure it could read the right size
		ifo_bytes_read = DVDReadBytes(ifo->file, buffer, (size_t)ifo_filesize);
		if(ifo_bytes_read != ifo_filesize) {
			printf(" * Bytes read and IFO filesize do not match: %ld, %ld\n", ifo_bytes_read, ifo_filesize);
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
		int vts_ifo_fd = -1;
		vts_ifo_fd = open(ifo_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(vts_ifo_fd == -1) {
			printf("* Could not create %s\n", ifo_filename);
			free(buffer);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		int vts_bup_fd = -1;
		vts_bup_fd = open(bup_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(vts_bup_fd == -1) {
			printf("* Could not open %s\n", bup_filename);
			free(buffer);
			close(vts_ifo_fd);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		ifo_bytes_written = write(vts_ifo_fd, buffer, (size_t)ifo_filesize);
		bup_bytes_written = write(vts_bup_fd, buffer, (size_t)ifo_filesize);

		// Check that source size and target sizes match
		if((ifo_bytes_written != ifo_filesize) || (bup_bytes_written != ifo_filesize) || (ifo_bytes_written != bup_bytes_written)) {
			if(ifo_bytes_written != ifo_filesize)
				printf("* IFO num bytes written and IFO filesize do not match: %ld, %ld\n", ifo_bytes_written, ifo_filesize);
			if(bup_bytes_written != ifo_filesize)
				printf("* BUP num bytes written and BUP filesize do not match: %ld, %ld\n", bup_bytes_written, ifo_filesize);
			if(ifo_bytes_written != bup_bytes_written)
				printf("* IFO num bytes written and BUP num bytes written do not match: %ld, %ld\n", ifo_bytes_written, bup_bytes_written);
			free(buffer);
			close(vts_ifo_fd);
			close(vts_bup_fd);
			ifoClose(ifo);
			DVDClose(dvdread_dvd);
			return 1;
		}

		// TODO: Check if the IFO and BUP file contents are exactly the same
		free(buffer);
		buffer = NULL;
		close(vts_ifo_fd);
		vts_ifo_fd = -1;
		close(vts_bup_fd);
		vts_bup_fd = -1;

		ifoClose(ifo);
		ifo = NULL;

	}
	*/

	// Get the filenames for everything on the DVD
	/*
	char vts_vob_filenames[100][10][22] = {'\0'};
	char vts_vob_filename[22] = {'\0'};
	*/


	// A DVD has a max of 99 * 10 VOBs
	/*
	uint16_t vts_index = 0;
	uint8_t vob_index = 0;
	for(vts_index = 0; vts_index < 99; vts_index++) {
		for(vob_index = 0; vob_index < 10; vob_index++) {
			snprintf(vts_vob_filenames[vts_index][vob_index], 22, "VIDEO_TS/VTS_%02u_%u.VOB", vts_index + 1, vob_index);
		}
	}
	*/

	// DVD
	struct dvd_info dvd_info;
	memset(dvd_info.dvdread_id, '\0', sizeof(dvd_info.dvdread_id));
	dvd_info.video_title_sets = 1;
	dvd_info.side = 1;
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	memset(dvd_info.provider_id, '\0', sizeof(dvd_info.provider_id));
	memset(dvd_info.vmg_id, '\0', sizeof(dvd_info.vmg_id));
	dvd_info.tracks = dvd_tracks(vmg_ifo);
	dvd_info.longest_track = 1;

	// Video Title Set
	struct dvd_vts dvd_vts;
	dvd_vts.vts = 1;
	memset(dvd_vts.id, '\0', sizeof(dvd_vts.id));
	dvd_vts.blocks = 0;
	dvd_vts.filesize = 0;
	dvd_vts.vobs = 0;

	// Video Title Set VOB
	struct dvd_vob dvd_vob;
	dvd_vob.vts = 1;
	dvd_vob.vob = 1;
	dvd_vob.blocks = 0;
	dvd_vob.filesize = 0;

	// Track
	struct dvd_track dvd_track;
	dvd_track.track = 1;
	dvd_track.valid = 1;
	dvd_track.vts = 1;
	dvd_track.ttn = 1;
	snprintf(dvd_track.length, DVD_TRACK_LENGTH + 1, "00:00:00.000");
	dvd_track.msecs = 0;
	dvd_track.chapters = 0;
	dvd_track.audio_tracks = 0;
	dvd_track.active_audio = 0;
	dvd_track.subtitles = 0;
	dvd_track.active_subs = 0;
	dvd_track.cells = 0;
	dvd_track.filesize = 0;

	// Chapters
	struct dvd_chapter dvd_chapter;
	dvd_chapter.chapter = 0;
	snprintf(dvd_chapter.length, DVD_CHAPTER_LENGTH + 1, "00:00:00.000");
	dvd_chapter.first_cell = 1;

	// Cells
	struct dvd_cell dvd_cell;
	dvd_cell.cell = 1;
	memset(dvd_cell.length, '\0', sizeof(dvd_cell.length));
	snprintf(dvd_cell.length, DVD_CELL_LENGTH + 1, "00:00:00.000");
	dvd_cell.msecs = 0;

	// Update from placeholder
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);

	ifo_handle_t *vts_ifos[dvd_info.video_title_sets + 1];
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

	// Open first IFO
	dvd_track.vts = 1;
	// printf("* Opening VTS IFO %u\n", dvd_track.vts);
	vts_ifo = ifoOpen(dvdread_dvd, dvd_track.vts);
	if(vts_ifo == NULL) {
		printf("* Could not open VTS_IFO for track %u\n", dvd_track.track);
		return 1;
	}
	ifoClose(vts_ifo);
	vts_ifo = NULL;

	// Find out the filesize of the VTS IFO
	dvdread_ifo_file = DVDOpenFile(dvdread_dvd, dvd_track.vts, DVD_READ_INFO_FILE);
	ifo_filesize = DVDFileSize(dvdread_ifo_file) * DVD_VIDEO_LB_LEN;
	// printf("* IFO filesize: %ld\n", ifo_filesize);

	/*
	dvdread_vts_file = DVDOpenFile(dvdread_dvd, dvd_track.vts, DVD_READ_TITLE_VOBS);

	dvd_vts.blocks = dvd_vts_blocks(dvdread_dvd, dvd_vts.vts);
	dvd_vts.filesize = dvd_vts_filesize(dvdread_dvd, dvd_vts.vts);
	dvd_vts.vobs = dvd_vts_vobs(dvdread_dvd, dvd_vts.vts);
	*/

	// Get the first menu VOB, if present
	// todo: Use DVDFileStat to see if it exists, then copy it manually (after titles)
	// Skipping copying menu VOBs completely right now
	dvdread_menu_file = DVDOpenFile(dvdread_dvd, dvd_track.vts, DVD_READ_MENU_VOBS);
	menu_blocks = DVDFileSize(dvdread_menu_file);
	menu_filesize = menu_blocks * DVD_VIDEO_LB_LEN;
	// if(menu_filesize)
	//	printf("* Menu VOB filesize:		%ld\n", menu_filesize);

	// Find the size of the first VTS VOB
	// dvd_stat_t dvdread_stat;
	// Get the size of the Menu VOB (if any)
	// DVDFileStat(dvdread_dvd, 1, DVD_READ_MENU_VOBS, &dvdread_stat);
	// DVDFileStat(dvdread_dvd, 1, DVD_READ_TITLE_VOBS, &dvdread_stat);
	
	dvd_vob.filesize = dvd_vob_filesize(dvdread_dvd, dvd_vts.vts, dvd_vob.vob);

	dvd_info.tracks = dvd_tracks(vmg_ifo);

	// Exit if track number requested does not exist
	if(opt_track_number && (arg_track_number > dvd_info.tracks || arg_track_number < 1)) {
		fprintf(stderr, "dvd_copy: Invalid track number %d\n", arg_track_number);
		fprintf(stderr, "dvd_copy: Valid track numbers: 1 to %u\n", dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	} else if(opt_track_number) {
		d_first_track = (uint16_t)arg_track_number;
		d_last_track = (uint16_t)arg_track_number;
		track_number = d_first_track;
		d_all_tracks = false;
	} else {
		d_first_track = 1;
		d_last_track = dvd_info.tracks;
		d_all_tracks = true;
	}

	dvd_vob.vob = 1;
	dvd_vob.vts = dvd_vts.vts;
	dvd_vob.blocks = dvd_vob_blocks(dvdread_dvd, dvd_vts.vts, dvd_vob.vob);

	// # of blocks to copy at once (top limit)
	dvd_read_blocks = DVD_COPY_BLOCK_LIMIT; // set to whatever to test on! :D
	read_blocks = 0; // This will adjust based on the boundaries encountered by dvd_read_blocks

	dvd_read_buffer = (unsigned char *)calloc(1, (uint64_t)DVD_COPY_BYTES_LIMIT * sizeof(unsigned char));
	if(dvd_read_buffer == NULL) {
		printf("Couldn't allocate memory\n");
		return 1;
	}

	// Handle for file that you write the DVD VOB out to
	// Filename here is VIDEO_TS/VTS_01_1.VOB
	/*
	if(copy_vobs) {
		vob_fd = open(vts_vob_filenames[dvd_vts.vts - 1][dvd_vob.vob], O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
		if(vob_fd == -1) {
			printf("Couldn't create file %s\n", vts_vob_filenames[vts_index][vob_index]);
			return 1;
		}
	}
	*/

	// A VTS can have multiple VOBs; Tracks = can span multiple VOBs and cells; Cells can also span multiple VOBs
	// Backing up the DVD using the cell sectors, which *possibly* is safer, because the DVD may jump around (I'd need data to prove that). You could also add checks to see if a certain sector has already been written as well. Possibly other nasty DVD authoring tricks to poison the well could be caught, too. Either way, the cell sectors are king since they point to filesystem locations.

	dvd_block_offset = 0;
	// vob_blocks_written = 0;
	// vts_blocks_written = 0;
	bytes_written = 0;

	/*
	track_fd = open("dvd_track.mpg", O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
	if(track_fd == -1) {
		printf("Couldn't create file %s\n", vts_vob_filenames[vts_index][vob_index]);
		return 1;
	}
	*/

	int cell_fd = 0;
	char cell_filename[33] = {'\0'};

	// for(dvd_track.track = 1; dvd_track.track < dvd_info.tracks + 1; dvd_track.track++) {
	for(dvd_track.track = d_first_track; dvd_track.track < d_last_track + 1; dvd_track.track++) {

		// Populate the track
		dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, dvd_track.track);
		dvd_vts.vts = dvd_track.vts;
		dvd_vob.vts = dvd_track.vts;
		vts_ifo = vts_ifos[dvd_track.vts];

		dvd_vts.blocks = dvd_vts_blocks(dvdread_dvd, dvd_vts.vts);
		dvd_vts.filesize = dvd_vts_filesize(dvdread_dvd, dvd_vts.vts);
		dvd_vts.vobs = dvd_vts_vobs(dvdread_dvd, dvd_vts.vts);
		// vts_blocks_written = 0;

		dvdread_vts_file = DVDOpenFile(dvdread_dvd, dvd_track.vts, DVD_READ_TITLE_VOBS);

		dvd_vts.blocks = dvd_vts_blocks(dvdread_dvd, dvd_vts.vts);
		dvd_vts.filesize = dvd_vts_filesize(dvdread_dvd, dvd_vts.vts);
		dvd_vts.vobs = dvd_vts_vobs(dvdread_dvd, dvd_vts.vts);

		dvd_track.chapters = dvd_track_chapters(vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.cells = dvd_track_cells(vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.blocks = dvd_track_blocks(vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.filesize = dvd_track_filesize(vmg_ifo, vts_ifo, dvd_track.track);
		dvd_track.audio_tracks = dvd_track_audio_tracks(vts_ifo);
		dvd_track.subtitles = dvd_track_subtitles(vts_ifo);
		strncpy(dvd_track.length, dvd_track_length(vmg_ifo, vts_ifo, dvd_track.track), DVD_TRACK_LENGTH);

		// printf("Track: %02u, Length: %s Chapters: %02u, Cells: %02u, Audio streams: %02u, Subpictures: %02u\n", dvd_track.track, dvd_track.length, dvd_track.chapters, dvd_track.cells, dvd_track.audio_tracks, dvd_track.subtitles);
		printf("Track: %02u\n* Filesize: %lu\n* Title set: %u\n", dvd_track.track, dvd_track.filesize, dvd_vts.vts);

		if(copy_tracks) {
			snprintf(track_filename, 24, "track_%02i_chap_%02i_%02i.mpg", dvd_track.track, 1, dvd_track.chapters);
			track_fd = open(track_filename, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
			if(track_fd == -1) {
				printf("Couldn't create file %s\n", track_filename);
				continue;
			}
		}

		track_blocks_written = 0;

		for(dvd_cell.cell = 1; dvd_cell.cell < dvd_track.cells + 1; dvd_cell.cell++) {

			dvd_cell.blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.filesize = dvd_cell_filesize(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			strncpy(dvd_cell.length, dvd_cell_length(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell), DVD_CELL_LENGTH);
			cell_sectors = dvd_cell.last_sector - dvd_cell.first_sector;

			printf("        Cell: %02u, Sectors: %u to %u\n", dvd_cell.cell, dvd_cell.first_sector, dvd_cell.last_sector);

			cell_blocks_written = 0;

			if(dvd_cell.last_sector > dvd_cell.first_sector)
				cell_sectors++;

			if(dvd_cell.last_sector < dvd_cell.first_sector) {
				printf("* DEBUG Someone doing something nasty? The last sector is listed before the first; skipping cell\n");
				continue;
			}
			
			dvd_block_offset = (int)dvd_cell.first_sector;

			/*
			if(dvd_cell.first_sector == dvd_cell.last_sector) {
				printf("* DEBUG check for first sector not being last sector failed; don't know if this is common or not\n");
				return 1;
			}
			*/

			if(copy_cells) {
				snprintf(cell_filename, 33, "track_%02i_chap_%02i_%02i_cell_%02i.mpg", dvd_track.track, 1, dvd_track.chapters, dvd_cell.cell);
				cell_fd = open(cell_filename, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
			}

			// while(cell_blocks_written < dvd_cell.blocks && vob_blocks_written < dvd_vob.blocks) {
			// This is where you would change the boundaries -- are you dumping to a track file (no boundaries) or a VOB (boundaries)
			while(cell_blocks_written < dvd_cell.blocks) {

				// Reset to the defaults
				read_blocks = DVD_COPY_BLOCK_LIMIT;

				if(read_blocks > (dvd_cell.blocks - cell_blocks_written)) {
					read_blocks = dvd_cell.blocks - cell_blocks_written;
				}

				/*
				if(copy_vobs) {
					if(read_blocks > (dvd_vob.blocks - vob_blocks_written)) {
						read_blocks = dvd_vob.blocks - vob_blocks_written;
					}
				}
				*/

				dvdread_read_blocks = DVDReadBlocks(dvdread_vts_file, dvd_block_offset, (uint64_t)read_blocks, dvd_read_buffer);
				if(!dvdread_read_blocks) {
					printf("* Could not read data from cell %u\n", dvd_cell.cell);
					return 1;
				}

				// Check to make sure the amount read was what we wanted
				if(dvdread_read_blocks != read_blocks) {
					printf("*** Asked for %ld and only got %ld\n", read_blocks, dvdread_read_blocks);
					return 1;
				}

				// Increment the offsets
				dvd_block_offset += dvdread_read_blocks;

				// Write the buffer to the VOB file
				if(copy_vobs)
					bytes_written = write(vob_fd, dvd_read_buffer, (uint64_t)(read_blocks * DVD_VIDEO_LB_LEN));
				// Write the buffer to the track file
				if(copy_tracks) {
					bytes_written = write(track_fd, dvd_read_buffer, (uint64_t)(read_blocks * DVD_VIDEO_LB_LEN));
				}
				// Write the buffer to the cell file
				if(copy_cells) {
					write(cell_fd, dvd_read_buffer, (uint64_t)(read_blocks * DVD_VIDEO_LB_LEN));
				}
				if(!bytes_written) {
					printf("* Could not write data from cell %u\n", dvd_cell.cell);
					return 1;
				}

				// Check to make sure we wrote as much as we asked for
				if(bytes_written != dvdread_read_blocks * DVD_VIDEO_LB_LEN) {
					printf("*** Tried to write %ld bytes and only wrote %ld instead\n", dvdread_read_blocks * DVD_VIDEO_LB_LEN, bytes_written);
					return 1;
				}

				// printf("bytes written: %lu\n", bytes_written);

				// Increment the amount of blocks written
				cell_blocks_written += dvdread_read_blocks;
				// vob_blocks_written += dvdread_read_blocks;
				// vts_blocks_written += dvdread_read_blocks;
				track_blocks_written += dvdread_read_blocks;

				// printf("Progress %lu%%\r", track_blocks_written * 100 / dvd_track.blocks);
				// fflush(stdout);

				/*
				if(copy_vobs)
					printf("* [VTS %i] [Track %i] [VOB %i] [Cell %i] Written %ld of %ld blocks\r", dvd_vts.vts, dvd_track.track, dvd_vob.vob, dvd_cell.cell, vob_blocks_written, dvd_vob.blocks); 

				if(copy_vobs && ((vob_blocks_written == dvd_vob.blocks && dvd_vob.vob < dvd_vts.vobs) || vts_blocks_written == dvd_vts.blocks)) {

					close(vob_fd);

					dvd_vob.vob++;
					
					snprintf(vts_vob_filename, 22, "VIDEO_TS/VTS_%02u_%u.VOB", dvd_vts.vts, dvd_vob.vob);

					vob_fd = open(vts_vob_filename, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
					if(vob_fd == -1) {
						printf("Couldn't create file %s\n", vts_vob_filename);
						return 1;
					}

					dvd_vob.blocks = dvd_vob_blocks(dvdread_dvd, dvd_vob.vts, dvd_vob.vob);
					dvd_vob.filesize = dvd_vob_filesize(dvdread_dvd, dvd_vob.vts, dvd_vob.vob);

					vob_blocks_written = 0;
					vts_blocks_written = 0;

				}
				*/

			}

			close(cell_fd);
		
		}


		if(copy_tracks) {
			close(track_fd);
			// close(cell_fd);
		}

		DVDCloseFile(dvdread_vts_file);

		printf("\n");
	
	}
	
	if(copy_vobs)
		close(vob_fd);

	if(vts_ifo)
		ifoClose(vts_ifo);
	
	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(ifo)
		ifoClose(ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);
	
	return 0;

}
