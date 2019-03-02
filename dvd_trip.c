#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#ifdef __linux__
#include <linux/cdrom.h>
#include "dvd_drive.h"
#endif
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
#include <mpv/client.h>
#ifndef DVD_INFO_VERSION
#define DVD_INFO_VERSION "1.3"
#endif

#define DVD_INFO_PROGRAM "dvd_trip"

int main(int, char **);
void dvd_track_info(struct dvd_track *dvd_track, const uint16_t track_number, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo);
void print_usage(char *binary);
void print_version(char *binary);

struct dvd_trip {
	uint16_t track;
	uint8_t first_chapter;
	uint8_t last_chapter;
	uint8_t first_cell;
	uint8_t last_cell;
	ssize_t filesize;
	char *filename;
	char *preset;
	uint8_t x265_preset;
	uint8_t x265_crf;
};

int main(int argc, char **argv) {

	/**
	 * Parse options
	 */

	bool verbose = false;
	bool debug = false;
	bool quiet = false;
	bool opt_track_number = false;
	bool opt_chapter_number = false;
	bool opt_filename = false;
	bool opt_quality = false;
	uint16_t arg_track_number = 0;
	int long_index = 0;
	int opt = 0;
	opterr = 1;
	const char *str_options;
	str_options = "c:dfho:t:q:QVvz";
	uint8_t arg_first_chapter = 1;
	uint8_t arg_last_chapter = 99;
	char *token = NULL;
	struct dvd_trip dvd_trip;
	char dvd_mpv_args[13] = {'\0'};
	char dvd_mpv_first_chapter[4] = {'\0'};
	char dvd_mpv_last_chapter[4] = {'\0'};
	mpv_handle *dvd_mpv;
	mpv_event *dvd_mpv_event;
	struct mpv_event_log_message *dvd_mpv_log_message = NULL;
	const char *output_filename = "trip_encode.mkv";

	struct option long_options[] = {

		{ "chapters", required_argument, 0, 'c' },
		{ "deinterlace", no_argument, 0, 'd' },
		{ "fullscreen", no_argument, 0, 'f' },
		{ "output", required_argument, 0, 'o' },
		{ "track", required_argument, 0, 't' },
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },
		{ "quality", required_argument, 0, 'q' },
		{ "quiet", no_argument, 0, 'Q' },
		{ "verbose", no_argument, 0, 'v' },
		{ "debug", no_argument, 0, 'z' },
		{ 0, 0, 0, 0 }

	};

	dvd_trip.track = 1;
	dvd_trip.first_chapter = 1;
	dvd_trip.last_chapter = 99;
	dvd_trip.first_cell = 1;
	dvd_trip.last_cell = 1;
	dvd_trip.x265_preset = 5;
	dvd_trip.x265_crf = 28;

	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {

		switch(opt) {

			case 'c':
				opt_chapter_number = true;
				token = strtok(optarg, "-"); {
					if(strlen(token) > 2) {
						fprintf(stderr, "Chapter range must be between 1 and 99\n");
						return 1;
					}
					arg_first_chapter = (uint8_t)strtoumax(token, NULL, 0);
				}

				token = strtok(NULL, "-");
				if(token != NULL) {
					if(strlen(token) > 2) {
						fprintf(stderr, "Chapter range must be between 1 and 99\n");
						return 1;
					}
					arg_last_chapter = (uint8_t)strtoumax(token, NULL, 0);
				}

				if(arg_first_chapter == 0)
					arg_first_chapter = 1;
				if(arg_last_chapter < arg_first_chapter)
					arg_last_chapter = arg_first_chapter;

				break;

			case 'h':
				print_usage(DVD_INFO_PROGRAM);
				return 0;

			case 'q':

				if(strncmp(optarg, "low", 3) == 0) {
					opt_quality = true;
					dvd_trip.x265_preset = 4;
				} else if(strncmp(optarg, "medium", 6) == 0) {
					opt_quality = true;
					dvd_trip.x265_preset = 5;
				} else if(strncmp(optarg, "high", 4) == 0) {
					opt_quality = true;
					dvd_trip.x265_preset = 6;
					dvd_trip.x265_crf = 26;
				} else if(strncmp(optarg, "insane", 6) == 0) {
					opt_quality = true;
					dvd_trip.x265_preset = 7;
					dvd_trip.x265_crf = 20;
				} else {
					printf("dvd_trip: valid presets: low, medium, high, insane\n");
					return 1;
				}
				break;

			case 'Q':
				quiet = true;
				verbose = false;
				debug = false;
				break;

			case 'o':
				dvd_trip.filename = optarg;
				break;

			case 't':
				opt_track_number = true;
				arg_track_number = (uint16_t)strtoumax(optarg, NULL, 0);
				break;

			case 'V':
				print_version("dvd_trip");
				return 0;

			case 'v':
				verbose = true;
				break;

			case 'z':
				verbose = true;
				debug = true;
				break;

			// ignore unknown arguments
			case '?':
				print_usage("dvd_trip");
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

	// Check to see if device can be opened
	int dvd_fd = 0;
	dvd_fd = dvd_device_open(device_filename);
	if(dvd_fd < 0) {
		fprintf(stderr, "dvd_trip: error opening %s\n", device_filename);
		return 1;
	}
	dvd_device_close(dvd_fd);


#ifdef __linux__

	// Poll drive status if it is hardware
	if(dvd_device_is_hardware(device_filename)) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "drive status: ");
			dvd_drive_display_status(device_filename);

			return 1;

		}

	}

#endif

	dvd_reader_t *dvdread_dvd = NULL;
	dvdread_dvd = DVDOpen(device_filename);

	if(!dvdread_dvd) {
		fprintf(stderr, "* dvdread could not open %s\n", device_filename);
		return 1;
	}

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL) {
		fprintf(stderr, "* Could not open IFO zero\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// DVD
	struct dvd_info dvd_info;
	memset(dvd_info.dvdread_id, '\0', sizeof(dvd_info.dvdread_id));
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	dvd_info.side = 1;
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	memset(dvd_info.provider_id, '\0', sizeof(dvd_info.provider_id));
	memset(dvd_info.vmg_id, '\0', sizeof(dvd_info.vmg_id));
	dvd_info.tracks = dvd_tracks(vmg_ifo);
	dvd_info.longest_track = 1;

	dvd_title(dvd_info.title, device_filename);
	if(!quiet)
		printf("Disc title: %s\n", dvd_info.title);

	uint16_t num_ifos = 1;
	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	if(num_ifos < 1) {
		fprintf(stderr, "* DVD has no title IFOs?!\n");
		fprintf(stderr, "* Most likely a bug in libdvdread or a bad master or problems reading the disc\n");
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
		fprintf(stderr, "* Could not open VTS_IFO for track %u\n", 1);
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
		fprintf(stderr, "dvd_trip: Invalid track number %d\n", arg_track_number);
		fprintf(stderr, "dvd_trip: Valid track numbers: 1 to %u\n", dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	} else if(opt_track_number) {
		dvd_trip.track = arg_track_number;
	}

	uint16_t ix = 0;
	uint16_t track = 1;
	
	uint32_t longest_msecs = 0;
	
	for(ix = 0, track = 1; ix < dvd_info.tracks; ix++, track++) {
 
		vts = dvd_vts_ifo_number(vmg_ifo, ix + 1);
		vts_ifo = vts_ifos[vts];
		dvd_track_info(&dvd_tracks[ix], track, vmg_ifo, vts_ifo);

		if(dvd_tracks[ix].msecs > longest_msecs) {
			dvd_info.longest_track = track;
			longest_msecs = dvd_tracks[ix].msecs;
		}

	}

	// Set the track number to rip if none is passed as an argument
	if(!opt_track_number)
		dvd_trip.track = dvd_info.longest_track;
	
	dvd_track = dvd_tracks[dvd_trip.track - 1];

	// Set the proper chapter range
	if(opt_chapter_number) {
		if(arg_first_chapter > dvd_track.chapters) {
			dvd_trip.first_chapter = dvd_track.chapters;
			fprintf(stderr, "Resetting first chapter to %u\n", dvd_trip.first_chapter);
		} else
			dvd_trip.first_chapter = arg_first_chapter;
		
		if(arg_last_chapter > dvd_track.chapters) {
			dvd_trip.last_chapter = dvd_track.chapters;
			fprintf(stderr, "Resetting last chapter to %u\n", dvd_trip.last_chapter);
		} else
			dvd_trip.last_chapter = arg_last_chapter;
	} else {
		dvd_trip.first_chapter = 1;
		dvd_trip.last_chapter = dvd_track.chapters;
	}
	
	/**
	 * File descriptors and filenames
	 */
	dvd_file_t *dvdread_vts_file = NULL;

	vts = dvd_vts_ifo_number(vmg_ifo, dvd_trip.track);
	vts_ifo = vts_ifos[vts];

	// Open the VTS VOB
	dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_TITLE_VOBS);

	if(!quiet)
		printf("Track: %02u, Length: %s, Chapters: %02u, Cells: %02u, Audio streams: %02u, Subpictures: %02u, Filesize: %lu, Blocks: %lu\n", dvd_track.track, dvd_track.length, dvd_track.chapters, dvd_track.cells, dvd_track.audio_tracks, dvd_track.subtitles, dvd_track.filesize, dvd_track.blocks);

	dvd_trip.first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, dvd_trip.track, dvd_trip.first_chapter);
	dvd_trip.last_cell = dvd_chapter_last_cell(vmg_ifo, vts_ifo, dvd_trip.track, dvd_trip.last_chapter);


	// Set output frames per second based on source (NTSC or PAL)
	char mpv_ofps[] = "30000/1001";
	if(dvd_track_pal_video(vts_ifo))
		snprintf(mpv_ofps, 3, "25");
	if(verbose)
		printf("dvd_trip: output frames per second: %s\n", mpv_ofps);

	// Set default filename of "trip_encode.mkv"
	if(dvd_trip.filename == NULL) {
		dvd_trip.filename = calloc(16, sizeof(unsigned char));
		strncpy(dvd_trip.filename, "trip_encode.mkv", 16);
	}

	// libx265 configuration
	char x265_log_level[5] = {'\0'};
	if(quiet)
		strncpy(x265_log_level, "none", 5);
	else if (debug)
		strncpy(x265_log_level, "full", 5);
	else if (verbose)
		strncpy(x265_log_level, "info", 5);
	else
		strncpy(x265_log_level, "info", 5);

	char x265_preset[7] = {'\0'};
	switch(dvd_trip.x265_preset) {
		case 4:
			strncpy(x265_preset, "fast", 5);
			break;
		case 5:
			strncpy(x265_preset, "medium", 7);
			break;
		case 6:
			strncpy(x265_preset, "slow", 5);
			break;
		case 7:
			strncpy(x265_preset, "slower", 7);
			break;
		default:
			strncpy(x265_preset, "medium", 7);
			break;
	}

	if(verbose) {
		printf("dvd_trip: x265 preset: %s\n", x265_preset);
		printf("dvd_trip: x265 crf: %02u\n", dvd_trip.x265_crf);
		printf("dvd_trip: x265 log level: %s\n", x265_log_level);
	}

	// DVD playback using libmpv
	dvd_mpv = mpv_create();

	// Verbose output
	if(debug)
		mpv_request_log_messages(dvd_mpv, "debug");
	else if(verbose)
		mpv_request_log_messages(dvd_mpv, "v");
	else if(quiet)
		mpv_request_log_messages(dvd_mpv, "no");
	else
		mpv_request_log_messages(dvd_mpv, "info");

	// mpv zero-indexes tracks
	sprintf(dvd_mpv_args, "dvdread://%02u", dvd_trip.track - 1);

	// mpv's chapter range starts at the first one, and ends at the last one plus one
	// fex: to play chapter 1 only, mpv --start '#1' --end '#2'
	sprintf(dvd_mpv_first_chapter, "#%02u", dvd_trip.first_chapter);
	sprintf(dvd_mpv_last_chapter, "#%02u", dvd_trip.last_chapter + 1);

	// MPV uses zero-indexing for tracks, dvd_info uses one instead
	const char *dvd_mpv_commands[] = {
		"loadfile",
		dvd_mpv_args,
		NULL
	};

	mpv_set_option_string(dvd_mpv, "dvd-device", device_filename);
	mpv_set_option_string(dvd_mpv, "start", dvd_mpv_first_chapter);
	mpv_set_option_string(dvd_mpv, "end", dvd_mpv_last_chapter);
	mpv_set_option_string(dvd_mpv, "track-auto-selection", "yes");
	mpv_set_option_string(dvd_mpv, "input-default-bindings", "yes");
	mpv_set_option_string(dvd_mpv, "input-vo-keyboard", "yes");
	mpv_set_option_string(dvd_mpv, "resume-playback", "no");

	// Default DVD encoding options
	mpv_set_option_string(dvd_mpv, "o", dvd_trip.filename);
	mpv_set_option_string(dvd_mpv, "vf", "lavfi=yadif");
	mpv_set_option_string(dvd_mpv, "ovc", "libx265");
	mpv_set_option_string(dvd_mpv, "oac", "libfdk_aac");
	mpv_set_option_string(dvd_mpv, "ofps", mpv_ofps);

	char ovcopts[109] = {'\0'};
	snprintf(ovcopts, 109, "preset=%s,crf=%02u,x265-params=log-level=%s:colorprim=smpte170m:transfer=smpte170m:colormatrix=smpte170m", x265_preset, dvd_trip.x265_crf, x265_log_level);

	if(debug)
		printf("dvd_trip: mpv ovcopts: %s\n", ovcopts);

	mpv_set_option_string(dvd_mpv, "ovcopts", ovcopts);

	mpv_initialize(dvd_mpv);
	mpv_command(dvd_mpv, dvd_mpv_commands);

	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		// if(dvd_mpv_event->event_id != MPV_EVENT_LOG_MESSAGE)
		//	printf("mpv_event_name: %s\n", mpv_event_name(dvd_mpv_event->event_id));

		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN || dvd_mpv_event->event_id == MPV_EVENT_END_FILE)
			break;

		if(dvd_mpv_event->event_id == MPV_EVENT_LOG_MESSAGE) {
			dvd_mpv_log_message = (struct mpv_event_log_message *)dvd_mpv_event->data;
			printf("mpv [%s]: %s", dvd_mpv_log_message->level, dvd_mpv_log_message->text);
		}

		/*
		if(dvd_mpv_event->event_id == MPV_EVENT_CHAPTER_CHANGE)
			printf("dvd_trip: changing chapters\n");
		*/

	}

	mpv_terminate_destroy(dvd_mpv);

	DVDCloseFile(dvdread_vts_file);

	if(vts_ifo)
		ifoClose(vts_ifo);
	
	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);
	
	return 0;

}

void dvd_track_info(struct dvd_track *dvd_track, const uint16_t track_number, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo) {

	dvd_track->track = track_number;
	dvd_track->valid = 1;
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

}

void print_usage(char *binary) {

	printf("%s %s - a tiny DVD ripper\n", binary, DVD_INFO_VERSION);
	printf("\n");
	printf("Usage: %s [-t track] [-c chapter[-chapter]] [dvd path] [options]\n", binary);
	printf("\n");
	// printf("Options:\n");
	// printf("\n");
	printf("DVD path can be a device name, a single file, or directory.\n");
	printf("\n");
	printf("Examples:\n");
	printf("  dvd_trip			# Read default DVD device (%s)\n", DEFAULT_DVD_DEVICE);
	printf("  dvd_trip /dev/dvd		# Read a specific DVD device\n");
	printf("  dvd_trip video.iso    	# Read an image file\n");
	printf("  dvd_trip ~/Videos/DVD	# Read a directory that contains VIDEO_TS\n");
	printf("\n");

}

void print_version(char *binary) {

	printf("%s %s - http://dvds.beandog.org/ - (c) 2018 Steve Dibb <steve.dibb@gmail.com>, licensed under GPL-2\n", binary, DVD_INFO_VERSION);

}
