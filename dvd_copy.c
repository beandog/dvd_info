#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <inttypes.h>
#include <linux/cdrom.h>
#include <linux/limits.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
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
	printf("  dvd_info " DEFAULT_DVD_DEVICE "	# Read a DVD drive directly\n");
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

void dvd_track_info(struct dvd_track *dvd_track, const uint16_t track_number, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo);

void dvd_track_info(struct dvd_track *dvd_track, const uint16_t track_number, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo) {

	dvd_track->track = track_number;
	dvd_track->valid = 1;
	dvd_track->vts = dvd_vts_ifo_number(vmg_ifo, track_number);
	dvd_track->ttn = dvd_track_ttn(vmg_ifo, track_number);
	strncpy(dvd_track->length, dvd_track_length(vmg_ifo, vts_ifo, track_number), DVD_TRACK_LENGTH);
	dvd_track->msecs = dvd_track_msecs(vmg_ifo, vts_ifo, track_number);
	dvd_track->chapters = dvd_track_chapters(vmg_ifo, vts_ifo, track_number);
	dvd_track->audio_tracks = dvd_track_audio_tracks(vts_ifo);
	dvd_track->subtitles = dvd_track_subtitles(vts_ifo);
	dvd_track->active_audio = dvd_audio_active_tracks(vmg_ifo, vts_ifo, track_number);
	dvd_track->active_subs = dvd_track_active_subtitles(vmg_ifo, vts_ifo, track_number);
	dvd_track->cells = dvd_track_cells(vmg_ifo, vts_ifo, track_number);
	dvd_track->blocks = dvd_track_blocks(vmg_ifo, vts_ifo, track_number);
	dvd_track->filesize = dvd_track_filesize(vmg_ifo, vts_ifo, track_number);

}

int main(int argc, char **argv) {

	/**
	 * Parse options
	 */

	bool copy_tracks = false;
	bool copy_cells = false;
	bool opt_track_number = false;
	int arg_track_number = 0;
	uint16_t d_first_track = 1;
	uint16_t d_last_track = 1;
	bool d_all_tracks = false;
	int long_index = 0;
	int opt = 0;
	opterr = 1;
	bool opt_first_chapter = false;
	bool opt_last_chapter = false;
	uint8_t arg_first_chapter = 0;
	uint8_t arg_last_chapter = 0;
	const char *str_options;
	str_options = "c:d:ht:";

	struct option long_options[] = {

		{ "chapters", required_argument, 0, 'c' },
		{ "track", required_argument, 0, 't' },
		{ 0, 0, 0, 0 }

	};

	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {

		switch(opt) {

			case 'c':
				opt_first_chapter = true;
				if(strlen(optarg) > 2)
					arg_first_chapter = 1;
				if(!isdigit(optarg[0]))
					arg_first_chapter = 1;
				if(strlen(optarg) == 2 && !isdigit(optarg[1]))
					arg_first_chapter = 1;
				if(optarg[0] < '1')
					arg_first_chapter = 1;
				if(strlen(optarg) == 1)
					arg_first_chapter = (uint8_t)(optarg[0]) - 48;
				if(strlen(optarg) == 2)
					arg_first_chapter = (((uint8_t)(optarg[0]) - 48) * 10) + ((uint8_t)(optarg[1]) - 48);
				break;

			case 'd':
				opt_last_chapter = true;
				if(strlen(optarg) > 2)
					arg_last_chapter = 0;
				if(!isdigit(optarg[0]))
					arg_last_chapter = 0;
				if(strlen(optarg) == 2 && !isdigit(optarg[1]))
					arg_last_chapter = 0;
				if(optarg[0] < '1')
					arg_last_chapter = 0;
				if(strlen(optarg) == 1)
					arg_last_chapter = (uint8_t)(optarg[0]) - 48;
				if(strlen(optarg) == 2)
					arg_last_chapter = (((uint8_t)(optarg[0]) - 48) * 10) + ((uint8_t)(optarg[1]) - 48);
				break;

			case 'h':
				print_usage("dvd_copy");
				return 0;

			case 't':
				if(strlen(optarg) == 1) {
					if(optarg[0] < '1' || optarg[0] > '9')
						printf("Invalid track range: %s, must be between 1 and 99\n", optarg);
					else
						arg_track_number = (uint8_t)(optarg[0]) - 48;
				} else if(strlen(optarg) == 2) {
					if((optarg[0] < '1' || optarg[0] > '9') || (optarg[0] < '0' || optarg[0] > '9'))
						printf("Invalid track range: %s, must be between 1 and 99\n", optarg);
					else
						arg_track_number = ((uint8_t)(optarg[0] - 48) * 10) + ((uint8_t)(optarg[1] - 48));
				}
				opt_track_number = true;
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

	const char *device_filename = DEFAULT_DVD_DEVICE;

	if (argv[optind])
		device_filename = argv[optind];

	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "cannot access %s\n", device_filename);
		return 1;
	}

	int dvd_fd = 0;
	int drive_status;

	dvd_fd = open(device_filename, O_RDONLY | O_NONBLOCK);
	if(dvd_fd < 0) {
		fprintf(stderr, "error opening %s\n", device_filename);
		return 1;
	}
	drive_status = ioctl(dvd_fd, CDROM_DRIVE_STATUS);
	close(dvd_fd);

	// Poll drive status if it is hardware
	bool is_hardware = false;

	if(strncmp(device_filename, "/dev/", 5) == 0)
		is_hardware = true;

	if(is_hardware) {

		if(drive_status != CDS_DISC_OK) {

			switch(drive_status) {
				case 1:
					fprintf(stderr, "DVD drive %s has no disc\n", device_filename);
					break;
				case 2:
					fprintf(stderr, "DVD drive %s reports tray as open\n", device_filename);
					break;
				case 3:
					fprintf(stderr, "DVD drive %s reports as not ready\n", device_filename);
					break;
				default:
					fprintf(stderr, "Cannot poll DVD drive %s\n", device_filename);
					break;
			}

			return 1;
		}
	}

	dvd_reader_t *dvdread_dvd = NULL;
	dvdread_dvd = DVDOpen(device_filename);

	if(!dvdread_dvd) {
		printf("* dvdread could not open %s\n", device_filename);
		return 1;
	}

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL) {
		printf("* Could not open IFO zero\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	printf("Disc Title: %s\n", dvd_title(device_filename));

	uint16_t num_ifos = 1;
	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	if(num_ifos < 1) {
		printf("* DVD has no title IFOs?!\n");
		printf("* Most likely a bug in libdvdread or a bad master or problems reading the disc\n");
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}

	// DVD
	struct dvd_info dvd_info;
	memset(&dvd_info, 0, sizeof(dvd_info));
	dvd_info.tracks = dvd_tracks(vmg_ifo);
	dvd_info.longest_track = 1;
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	dvd_info.tracks = dvd_tracks(vmg_ifo);

	// Track
	struct dvd_track dvd_track;
	memset(&dvd_track, 0, sizeof(dvd_track));

	struct dvd_track dvd_tracks[dvd_info.tracks];
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
		printf("* Could not open VTS_IFO for track %u\n", 1);
		return 1;
	}
	ifoClose(vts_ifo);
	vts_ifo = NULL;

	// Create an array of all the IFOs
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
	

	// Exit if track number requested does not exist
	uint16_t track_number = 1;

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


	/**
	 * Integers for numbers of blocks read, copied, counters
	 */
	int offset = 0;
	ssize_t read_blocks = DVD_COPY_BLOCK_LIMIT; // this can be changed, if you want to to do testing on different block sizes (grab more / less data on each read)
	ssize_t dvdread_read_blocks = 0; // num blocks passed by dvdread function
	ssize_t cell_blocks_written = 0;
	ssize_t track_blocks_written = 0;
	ssize_t bytes_written = 0;
	uint32_t cell_sectors;

	unsigned char *dvd_read_buffer = NULL;
	dvd_read_buffer = (unsigned char *)calloc(1, (uint64_t)DVD_COPY_BYTES_LIMIT * sizeof(unsigned char));
	if(dvd_read_buffer == NULL) {
		printf("Couldn't allocate memory\n");
		return 1;
	}

	/**
	 * Populate array of DVD tracks
	 */
	uint16_t ix = 0;
	uint16_t track = 1;
	
	uint8_t c = 0;
	uint8_t cell = 1;
	
	for(ix = 0, track = 1; ix < dvd_info.tracks; ix++, track++) {
 
		vts = dvd_vts_ifo_number(vmg_ifo, ix + 1);
		vts_ifo = vts_ifos[vts];
		dvd_track_info(&dvd_tracks[ix], track, vmg_ifo, vts_ifo);

		// printf("Track: %02u, Length: %s Chapters: %02u, Cells: %02u, Audio streams: %02u, Subpictures: %02u\n", track, dvd_tracks[ix].length, dvd_tracks[ix].chapters, dvd_tracks[ix].cells, dvd_tracks[ix].audio_tracks, dvd_tracks[ix].subtitles);

	}

	// Check chapter number options, arguments, and boundaries
	if(arg_first_chapter == 0 && arg_last_chapter == 0) {
		arg_first_chapter = 1;
		arg_last_chapter = 99;
	}

	if(arg_last_chapter < arg_first_chapter)
		arg_last_chapter = arg_first_chapter;

	if(arg_first_chapter == 0)
		arg_first_chapter = 1;

	if(arg_last_chapter == 0)
		arg_last_chapter = arg_first_chapter;

	uint8_t first_chapter = 1;
	uint8_t last_chapter = dvd_tracks[track_number - 1].chapters;

	if(opt_first_chapter)
		first_chapter = arg_first_chapter;
	
	if(opt_last_chapter)
		last_chapter = arg_last_chapter;
	
	if(opt_first_chapter && (arg_first_chapter > dvd_tracks[track_number - 1].chapters))
		first_chapter = dvd_tracks[track_number - 1].chapters;

	if(opt_last_chapter && (arg_last_chapter > dvd_tracks[track_number - 1].chapters))
		last_chapter = dvd_tracks[track_number - 1].chapters;

	/**
	 * File descriptors and filenames
	 */
	dvd_file_t *dvdread_vts_file = NULL;

	int track_fd = 0;
	char track_filename[18] = {'\0'};

	int cell_fd = 0;
	char cell_filename[26] = {'\0'};

	/**
	 * Copy selected tracks
	 */

	for(track = d_first_track, ix = d_first_track - 1; track < d_last_track + 1; track++, ix++) {

		// Populate the track
		vts = dvd_vts_ifo_number(vmg_ifo, track);
		vts_ifo = vts_ifos[vts];

		// Open the VTS VOB
		dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_TITLE_VOBS);

		printf("Track: %02u, Length: %s Chapters: %02u, Cells: %02u, Audio streams: %02u, Subpictures: %02u, Filesize: %lu, Blocks: %lu\n", dvd_tracks[ix].track, dvd_tracks[ix].length, dvd_tracks[ix].chapters, dvd_tracks[ix].cells, dvd_tracks[ix].audio_tracks, dvd_tracks[ix].subtitles, dvd_tracks[ix].filesize, dvd_tracks[ix].blocks);

		if(copy_tracks) {
			snprintf(track_filename, 17, "dvd_track_%02i.vob", track);
			track_fd = open(track_filename, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
			if(track_fd == -1) {
				printf("Couldn't create file %s\n", track_filename);
				continue;
			}
		}

		track_blocks_written = 0;

		for(c = 0, cell = 1; c < dvd_tracks[ix].cells; c++, cell++) {

			dvd_cell.cell = cell;
			dvd_cell.blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, dvd_tracks[ix].track, dvd_cell.cell);
			dvd_cell.filesize = dvd_cell_filesize(vmg_ifo, vts_ifo, dvd_tracks[ix].track, dvd_cell.cell);
			dvd_cell.first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, dvd_tracks[ix].track, dvd_cell.cell);
			dvd_cell.last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, dvd_tracks[ix].track, dvd_cell.cell);
			strncpy(dvd_cell.length, dvd_cell_length(vmg_ifo, vts_ifo, dvd_tracks[ix].track, dvd_cell.cell), DVD_CELL_LENGTH);
			cell_sectors = dvd_cell.last_sector - dvd_cell.first_sector;

			printf("        Cell: %02u, Filesize: %lu, Blocks: %lu, Sectors: %i to %i\n", dvd_cell.cell, dvd_cell.filesize, dvd_cell.blocks, dvd_cell.first_sector, dvd_cell.last_sector);

			cell_blocks_written = 0;

			if(dvd_cell.last_sector > dvd_cell.first_sector)
				cell_sectors++;

			if(dvd_cell.last_sector < dvd_cell.first_sector) {
				printf("* DEBUG Someone doing something nasty? The last sector is listed before the first; skipping cell\n");
				continue;
			}
			
			offset = (int)dvd_cell.first_sector;

			if(copy_cells) {
				snprintf(cell_filename, 26, "dvd_track_%02i_cell_%02i.vob", dvd_tracks[ix].track, dvd_cell.cell);
				cell_fd = open(cell_filename, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
			}

			// This is where you would change the boundaries -- are you dumping to a track file (no boundaries) or a VOB (boundaries)
			while(cell_blocks_written < dvd_cell.blocks) {

				// Reset to the defaults
				read_blocks = DVD_COPY_BLOCK_LIMIT;

				if(read_blocks > (dvd_cell.blocks - cell_blocks_written)) {
					read_blocks = dvd_cell.blocks - cell_blocks_written;
				}

				dvdread_read_blocks = DVDReadBlocks(dvdread_vts_file, offset, (uint64_t)read_blocks, dvd_read_buffer);
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
				offset += dvdread_read_blocks;

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

				// Increment the amount of blocks written
				cell_blocks_written += dvdread_read_blocks;
				track_blocks_written += dvdread_read_blocks;

				printf("Progress %lu%%\r", track_blocks_written * 100 / dvd_tracks[ix].blocks);
				fflush(stdout);

			}

			if(copy_cells)
				close(cell_fd);
		
		}

		if(copy_tracks)
			close(track_fd);

		DVDCloseFile(dvdread_vts_file);

		printf("\n");
	
	}
	
	if(vts_ifo)
		ifoClose(vts_ifo);
	
	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);
	
	return 0;

}
