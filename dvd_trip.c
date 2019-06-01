#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
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
#include "dvd_vts.h"
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
#define DVD_INFO_VERSION "1.6_beta1"
#endif

	/**
	 *      _          _    _        _
	 *   __| |_   ____| |  | |_ _ __(_)_ __
	 *  / _` \ \ / / _` |  | __| '__| | '_ \
	 * | (_| |\ V / (_| |  | |_| |  | | |_) |
	 *  \__,_| \_/ \__,_|___\__|_|  |_| .__/
	 * 	           |_____|        |_|
	 *
	 * ** the tiniest little dvd ripper you'll ever see :) **
	 *
	 * About
	 *
	 * dvd_trip is designed to be a tiny DVD ripper with very few options. The purpose being
	 * that there is as little overhead as possible when wanting to do a quick DVD rip, and
	 * has a good set of default encoding settings.
	 *
	 * Just running "dvd_trip" alone with no arguments or options will fetch the longest track
	 * on the DVD, select the first English audio track, and encode it to H.265 using libx265 with
	 * its medium preset and default CRF of 28, and AAC audio using libfdk-aac. It also preserves
	 * the source's framerate.
	 *
	 * Because it is so tiny, some options it doesn't have that larger DVD ripper applications
	 * would are: encoding multiple audio streams, audio passthrough, subtitle support (VOBSUB and
	 * closed captioning), auto-cropping black bars, and custom audio and video codec options.
	 *
	 * However(!), if you want more features when ripping the DVD, you can use "dvd_copy" instead,
	 * and output the track to stdout. Using ffmpeg / libav or otherwise can detect the streams
	 * and encode, crop, copy, and so on directly. Example: dvd_copy -o - | ffprobe -i -
	 *
	 * Presets
	 *
	 * To keep things simple, dvd_trip only has three presets which fit the most commonly used
	 * codecs and formats when ripping DVDs: "mkv", "mp4", "webp".
	 *
	 * Quality Levels
	 *
	 * The quality levels of dvd_trip could partially be described as the time it takes to encode
	 * them, rather than an intentional target towards a quality level. In the case of the "low"
	 * presets, the focus is simply to go faster. The "medium" preset uses the default encoding
	 * settings from the codecs themselves. The "high" and "insane" presets bump up the bitrates
	 * from the defaults and will make the encoding slower, while also looking much nicer.
	 *
	 */

int main(int, char **);
void dvd_track_info(struct dvd_track *dvd_track, const uint16_t track_number, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo);

struct dvd_trip {
	uint16_t track;
	uint8_t first_chapter;
	uint8_t last_chapter;
	ssize_t filesize;
	char filename[PATH_MAX - 1];
	char container[5];
	char preset[7];
	char vcodec[256];
	char vcodec_preset[11];
	char vcodec_opts[256];
	char vcodec_log_level[6];
	char color_opts[256];
	char audio_lang[3];
	char audio_aid[4];
	char acodec[256];
	char acodec_opts[256];
	char vf_opts[256];
	uint8_t crf;
	char fps[11];
	bool deinterlace;
	uint8_t pass;
};

int main(int argc, char **argv) {

	/**
	 * Parse options
	 */

	bool verbose = false;
	bool debug = false;
	bool invalid_opts = false;
	bool opt_track_number = false;
	bool opt_chapter_number = false;
	bool opt_filename = false;
	uint16_t arg_track_number = 1;
	int long_index = 0;
	int opt = 0;
	unsigned long int arg_number = 0;
	uint8_t arg_first_chapter = 1;
	uint8_t arg_last_chapter = 99;
	char *token = NULL;
	char *token_filename = NULL;
	char tmp_filename[5] = {'\0'};
	char dvd_mpv_args[13] = {'\0'};
	char dvd_mpv_first_chapter[5] = {'\0'};
	char dvd_mpv_last_chapter[5] = {'\0'};
	mpv_handle *dvd_mpv = NULL;
	mpv_event *dvd_mpv_event = NULL;
	struct mpv_event_log_message *dvd_mpv_log_message = NULL;

	// Video Title Set
	struct dvd_vts dvd_vts[99];

	struct dvd_trip dvd_trip;

	dvd_trip.track = 1;
	dvd_trip.first_chapter = 1;
	dvd_trip.last_chapter = 99;

	memset(dvd_mpv_first_chapter, '\0', sizeof(dvd_mpv_first_chapter));
	memset(dvd_mpv_last_chapter, '\0', sizeof(dvd_mpv_last_chapter));

	memset(dvd_trip.filename, '\0', sizeof(dvd_trip.filename));
	memset(dvd_trip.container, '\0', sizeof(dvd_trip.container));
	strcpy(dvd_trip.container, "mkv");
	memset(dvd_trip.preset, '\0', sizeof(dvd_trip.preset));
	strcpy(dvd_trip.preset, "medium");
	memset(dvd_trip.vcodec, '\0', sizeof(dvd_trip.vcodec));
	memset(dvd_trip.vcodec_preset, '\0', sizeof(dvd_trip.vcodec_preset));
	memset(dvd_trip.vcodec_opts, '\0', sizeof(dvd_trip.vcodec_opts));
	memset(dvd_trip.vcodec_log_level, '\0', sizeof(dvd_trip.vcodec_log_level));
	memset(dvd_trip.color_opts, '\0', sizeof(dvd_trip.color_opts));
	memset(dvd_trip.acodec, '\0', sizeof(dvd_trip.acodec));
	memset(dvd_trip.acodec_opts, '\0', sizeof(dvd_trip.acodec_opts));
	memset(dvd_trip.audio_lang, '\0', sizeof(dvd_trip.audio_lang));
	memset(dvd_trip.audio_aid, '\0', sizeof(dvd_trip.audio_aid));
	memset(dvd_trip.vf_opts, '\0', sizeof(dvd_trip.vf_opts));
	dvd_trip.crf = 28;
	memset(dvd_trip.fps, '\0', sizeof(dvd_trip.fps));
	dvd_trip.deinterlace = false;
	dvd_trip.pass = 1;

	struct option long_options[] = {

		{ "chapters", required_argument, 0, 'c' },
		{ "track", required_argument, 0, 't' },
		{ "alang", required_argument, 0, 'l' },
		{ "aid", required_argument, 0, 'A' },

		{ "deinterlace", no_argument, 0, 'd' },
		{ "preset", required_argument, 0, 'p' },

		{ "output", required_argument, 0, 'o' },

		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },

		{ "verbose", no_argument, 0, 'v' },
		{ "debug", no_argument, 0, 'z' },
		{ 0, 0, 0, 0 }

	};

	while((opt = getopt_long(argc, argv, "Ac:dhl:o:p:t:vz", long_options, &long_index )) != -1) {

		switch(opt) {

			case 'A':
				strncpy(dvd_trip.audio_aid, optarg, 3);
				break;

			case 'c':
				opt_chapter_number = true;
				token = strtok(optarg, "-");
				if(strlen(token) > 2) {
					fprintf(stderr, "[dvd_trip] Chapter range must be between 1 and 99\n");
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
				if(token != NULL) {
					if(strlen(token) > 2) {
						fprintf(stderr, "[dvd_trip] Chapter range must be between 1 and 99\n");
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
				dvd_trip.deinterlace = true;
				break;

			case 'l':
				strncpy(dvd_trip.audio_lang, optarg, 2);
				break;

			case 'p':
				if(strncmp(optarg, "low", 3) == 0) {
					strcpy(dvd_trip.preset, "low");
				} else if(strncmp(optarg, "medium", 6) == 0) {
					strcpy(dvd_trip.preset, "medium");
				} else if(strncmp(optarg, "high", 4) == 0) {
					strcpy(dvd_trip.preset, "high");
				} else if(strncmp(optarg, "insane", 6) == 0) {
					strcpy(dvd_trip.preset, "insane");
				} else {
					printf("dvd_trip [error]: valid presets - low medium high insane\n");
					return 1;
				}
				break;

			case 'o':
				opt_filename = true;
				strncpy(dvd_trip.filename, optarg, PATH_MAX - 1);
				token_filename = strtok(optarg, ".");

				// Choose preset from file extension
				while(token_filename != NULL) {

					snprintf(tmp_filename, 5, "%s", token_filename);
					token_filename = strtok(NULL, ".");

					if(token_filename == NULL && strlen(tmp_filename) == 3 && strncmp(tmp_filename, "mkv", 3) == 0) {
						strncpy(dvd_trip.container, "mkv", 4);
					} else if(token_filename == NULL && strlen(tmp_filename) == 3 && strncmp(tmp_filename, "mp4", 3) == 0) {
						strncpy(dvd_trip.container, "mp4", 4);
					} else if(token_filename == NULL && strlen(tmp_filename) == 4 && strncmp(tmp_filename, "webm", 4) == 0) {
						strncpy(dvd_trip.container, "webm", 5);
					}

				}

				break;

			case 't':
				opt_track_number = true;
				arg_number = strtoul(optarg, NULL, 0);
				if(arg_number > 99)
					arg_track_number = 99;
				else if(arg_number == 0)
					arg_track_number = 1;
				else
					arg_track_number = (uint16_t)arg_number;
				break;

			case 'V':
				printf("dvd_trip %s\n", DVD_INFO_VERSION);
				return 0;

			case 'v':
				verbose = true;
				break;

			case 'z':
				verbose = true;
				debug = true;
				break;

			case '?':
				invalid_opts = true;
			case 'h':
				printf("dvd_trip - a tiny DVD ripper\n");
				printf("\n");
				printf("Usage:\n");
				printf("  dvd_trip [path] [options]\n");
				printf("\n");
				printf("Encoding ptions:\n");
				printf("  -t, --track <number>          Encode selected track (default: longest)\n");
				printf("  -c, --chapter <#>[-#]         Encode chapter number or range (default: all)\n");
				printf("  -o, --output <filename>       Save to filename (default: dvd_track_##.mkv)\n");
				printf("  -l, --alang <language>	Select audio language, two character code (default: first audio track)\n");
				printf("  -A, --aid <#> 		Select audio track ID\n");
				printf("\n");
				printf("  -h, --help			Show this help text and exit\n");
				printf("      --version			Show version info and exit\n");
				printf("\n");
				printf("DVD path can be a device name, a single file, or directory (default: %s)\n", DEFAULT_DVD_DEVICE);
				printf("\n");
				printf("dvd_trip reads a configuration file from ~/.config/dvd_trip/mpv.conf\n");
				printf("\n");
				printf("See mpv man page for syntax and dvd_trip man page for examples.\n");
				if(invalid_opts)
					return 1;
				return 0;

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

		dvd_vts[vts].vts = vts;
		dvd_vts[vts].valid = false;
		dvd_vts[vts].blocks = 0;
		dvd_vts[vts].filesize = 0;
		dvd_vts[vts].vobs = 0;
		dvd_vts[vts].tracks = 0;
		dvd_vts[vts].valid_tracks = 0;
		dvd_vts[vts].invalid_tracks = 0;

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(vts_ifos[vts] == NULL) {
			dvd_vts[vts].valid = false;
			vts_ifos[vts] = NULL;
		} else if(!ifo_is_vts(vts_ifos[vts])) {
			dvd_vts[vts].valid = false;
			ifoClose(vts_ifos[vts]);
			vts_ifos[vts] = NULL;
		} else {
			dvd_vts[vts].valid = true;
		}

	}
	
	// Exit if track number requested does not exist
	if(opt_track_number && (arg_track_number > dvd_info.tracks)) {
		fprintf(stderr, "[dvd_trip] Invalid track number %" PRIu16 "\n", arg_track_number);
		fprintf(stderr, "[dvd_trip] Valid track numbers: 1 to %" PRIu16 "\n", dvd_info.tracks);
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
		dvd_tracks[ix].valid = true;

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

	printf("Track: %02u, Length: %s, Chapters: %02u, Cells: %02u, Audio streams: %02u, Subpictures: %02u, Filesize: %lu, Blocks: %lu\n", dvd_track.track, dvd_track.length, dvd_track.chapters, dvd_track.cells, dvd_track.audio_tracks, dvd_track.subtitles, dvd_track.filesize, dvd_track.blocks);

	// Check for track issues
	dvd_track.valid = true;

	if(dvd_vts[vts].valid == false) {
		dvd_track.valid = false;
	}

	if(dvd_track.msecs == 0) {
		printf("	Error: track has zero length\n");
		dvd_track.valid = false;
	}

	if(dvd_track.chapters == 0) {
		printf("	Error: track has zero chapters\n");
		dvd_track.valid = false;
	}

	if(dvd_track.cells == 0) {
		printf("	Error: track has zero cells\n");
		dvd_track.valid = false;
	}

	if(dvd_track.valid == false) {

		printf("Track has been marked as invalid, quitting\n");

		DVDCloseFile(dvdread_vts_file);

		if(vts_ifo)
			ifoClose(vts_ifo);

		if(vmg_ifo)
			ifoClose(vmg_ifo);

		if(dvdread_dvd)
			DVDClose(dvdread_dvd);

		return 1;

	}

	// MPV zero-indexes tracks
	sprintf(dvd_mpv_args, "dvdread://%u", dvd_trip.track - 1);

	const char *dvd_mpv_commands[] = { "loadfile", dvd_mpv_args, NULL };


	// DVD playback using libmpv
	dvd_mpv = mpv_create();

	// Terminal output
	mpv_set_option_string(dvd_mpv, "terminal", "yes");
	if(!debug)
		mpv_set_option_string(dvd_mpv, "term-osd-bar", "yes");

	if (debug) {
		mpv_request_log_messages(dvd_mpv, "debug");
		strcpy(dvd_trip.vcodec_log_level, "full");
	} else if(verbose) {
		mpv_request_log_messages(dvd_mpv, "v");
		strcpy(dvd_trip.vcodec_log_level, "info");
	} else {
		mpv_request_log_messages(dvd_mpv, "info");
		strcpy(dvd_trip.vcodec_log_level, "info");
	}

	/** Video **/

	// Set output frames per second and color spaces based on source (NTSC or PAL)
	if(dvd_track_pal_video(vts_ifo)) {
		strcpy(dvd_trip.fps, "25");
		strcpy(dvd_trip.color_opts, "color_primaries=bt470bg,color_trc=gamma28,colorspace=bt470bg");
	} else {
		strcpy(dvd_trip.fps, "30000/1001");
		strcpy(dvd_trip.color_opts, "color_primaries=smpte170m,color_trc=smpte170m,colorspace=smpte170m");
	}

	/** Containers and Presets **/

	// Set preset defaults
	if(strncmp(dvd_trip.container, "mkv", 3) == 0) {

		if(!opt_filename)
			strcpy(dvd_trip.filename, "trip_encode.mkv");

		strcpy(dvd_trip.vcodec, "libx265");
		strcpy(dvd_trip.vcodec_preset, "medium");
		strcpy(dvd_trip.acodec, "libfdk_aac");


		if(strncmp(dvd_trip.preset, "low", 3) == 0) {
			strcpy(dvd_trip.vcodec_preset, "fast");
			dvd_trip.crf = 28;
		} else if(strncmp(dvd_trip.preset, "medium", 6) == 0) {
			strcpy(dvd_trip.vcodec_preset, "medium");
			dvd_trip.crf = 24;
		} else if(strncmp(dvd_trip.preset, "high", 4) == 0) {
			strcpy(dvd_trip.vcodec_preset, "slow");
			strcpy(dvd_trip.acodec_opts, "b=192k");
			dvd_trip.crf = 20;
		} else if(strncmp(dvd_trip.preset, "insane", 6) == 0) {
			strcpy(dvd_trip.vcodec_preset, "slower");
			strcpy(dvd_trip.acodec_opts, "b=256k");
			dvd_trip.crf = 14;
		}

		sprintf(dvd_trip.vcodec_opts, "%s,preset=%s,crf=%u,x265-params=log-level=%s", dvd_trip.color_opts, dvd_trip.vcodec_preset, dvd_trip.crf, dvd_trip.vcodec_log_level);

	}

	if(strncmp(dvd_trip.container, "mp4", 3) == 0) {

		if(!opt_filename)
			strcpy(dvd_trip.filename, "trip_encode.mp4");

		strcpy(dvd_trip.vcodec, "libx264");
		strcpy(dvd_trip.vcodec_preset, "medium");
		strcpy(dvd_trip.acodec, "libfdk_aac");
		strcpy(dvd_trip.acodec_opts, "");


		if(strncmp(dvd_trip.preset, "low", 3) == 0) {
			strcpy(dvd_trip.vcodec_preset, "fast");
			dvd_trip.crf = 28;
		} else if(strncmp(dvd_trip.preset, "medium", 6) == 0) {
			strcpy(dvd_trip.vcodec_preset, "medium");
			dvd_trip.crf = 22;
		} else if(strncmp(dvd_trip.preset, "high", 4) == 0) {
			strcpy(dvd_trip.vcodec_preset, "slow");
			strcpy(dvd_trip.acodec_opts, "b=192k");
			dvd_trip.crf = 20;
		} else if(strncmp(dvd_trip.preset, "insane", 6) == 0) {
			strcpy(dvd_trip.vcodec_preset, "slower");
			strcpy(dvd_trip.acodec_opts, "b=256k");
			dvd_trip.crf = 16;
		}

		// x264 doesn't allow passing log level (that I can see)
		sprintf(dvd_trip.vcodec_opts, "%s,preset=%s,crf=%u", dvd_trip.color_opts, dvd_trip.vcodec_preset, dvd_trip.crf);

	}

	if(strncmp(dvd_trip.container, "webm", 4) == 0) {

		if(!opt_filename)
			strcpy(dvd_trip.filename, "trip_encode.webm");

		strcpy(dvd_trip.vcodec, "libvpx-vp9");
		strcpy(dvd_trip.acodec, "libopus");
		strcpy(dvd_trip.acodec_opts, "application=audio");

		if(strncmp(dvd_trip.preset, "low", 3) == 0) {
			dvd_trip.crf = 34;
			sprintf(dvd_trip.vcodec_opts, "%s,b=0,crf=%u,keyint_min=0,g=360", dvd_trip.color_opts, dvd_trip.crf);
			strcpy(dvd_trip.acodec_opts, "application=audio,b=96000");
		}

		if(strncmp(dvd_trip.preset, "medium", 6) == 0) {
			dvd_trip.crf = 32;
			sprintf(dvd_trip.vcodec_opts, "%s,b=0,crf=%u,keyint_min=0,g=360", dvd_trip.color_opts, dvd_trip.crf);
			strcpy(dvd_trip.acodec_opts, "application=audio,b=144000");
		}

		if(strncmp(dvd_trip.preset, "high", 4) == 0) {
			dvd_trip.crf = 22;
			sprintf(dvd_trip.vcodec_opts, "%s,b=0,crf=%u,keyint_min=0,g=360", dvd_trip.color_opts, dvd_trip.crf);
			strcpy(dvd_trip.acodec_opts, "application=audio,b=192000");
		}

		if(strncmp(dvd_trip.preset, "insane", 6) == 0) {
			dvd_trip.crf = 16;
			sprintf(dvd_trip.vcodec_opts, "%s,b=0,crf=%u,keyint_min=0,g=360", dvd_trip.color_opts, dvd_trip.crf);
			strcpy(dvd_trip.acodec_opts, "application=audio,b=256000");
		}

	}

	mpv_set_option_string(dvd_mpv, "o", dvd_trip.filename);
	mpv_set_option_string(dvd_mpv, "ovc", dvd_trip.vcodec);
	mpv_set_option_string(dvd_mpv, "ovcopts", dvd_trip.vcodec_opts);
	mpv_set_option_string(dvd_mpv, "oac", dvd_trip.acodec);
	if(strlen(dvd_trip.acodec_opts) > 0)
		mpv_set_option_string(dvd_mpv, "oacopts", dvd_trip.acodec_opts);
	mpv_set_option_string(dvd_mpv, "dvd-device", device_filename);
	mpv_set_option_string(dvd_mpv, "track-auto-selection", "yes");
	mpv_set_option_string(dvd_mpv, "input-default-bindings", "yes");
	mpv_set_option_string(dvd_mpv, "input-vo-keyboard", "yes");
	mpv_set_option_string(dvd_mpv, "resume-playback", "no");

	// MPV's chapter range starts at the first one, and ends at the last one plus one
	// fex: to play chapter 1 only, mpv --start '#1' --end '#2'
	sprintf(dvd_mpv_first_chapter, "#%u", dvd_trip.first_chapter);
	sprintf(dvd_mpv_last_chapter, "#%u", dvd_trip.last_chapter + 1);
	mpv_set_option_string(dvd_mpv, "start", dvd_mpv_first_chapter);
	mpv_set_option_string(dvd_mpv, "end", dvd_mpv_last_chapter);

	if(strlen(dvd_trip.audio_aid) > 0)
		mpv_set_option_string(dvd_mpv, "aid", dvd_trip.audio_aid);
	else if(strlen(dvd_trip.audio_lang) > 0)
		mpv_set_option_string(dvd_mpv, "alang", dvd_trip.audio_lang);

	/** Video Filters **/

	if(mpv_client_api_version() <= MPV_MAKE_VERSION(1, 25)) {

		// Syntax up to 0.27.2
		mpv_set_option_string(dvd_mpv, "ofps", dvd_trip.fps);

		if(dvd_trip.deinterlace)
			sprintf(dvd_trip.vf_opts, "lavfi=yadif");

	} else {

		// Syntax starting in 0.29.1
		if(dvd_trip.deinterlace)
			sprintf(dvd_trip.vf_opts, "lavfi-yadif,fps=%s", dvd_trip.fps);
		else
			sprintf(dvd_trip.vf_opts, "fps=%s", dvd_trip.fps);

	}

	mpv_set_option_string(dvd_mpv, "vf", dvd_trip.vf_opts);

	if(dvd_trip.pass == 1) {
		fprintf(stderr, "dvd_trip [info]: dvd track %u\n", dvd_trip.track);
		fprintf(stderr, "dvd_trip [info]: chapters %u to %u\n", dvd_trip.first_chapter, dvd_trip.last_chapter);
		fprintf(stderr, "dvd_trip [info]: saving to %s\n", dvd_trip.filename);
		fprintf(stderr, "dvd_trip [info]: vcodec %s\n", dvd_trip.vcodec);
		fprintf(stderr, "dvd_trip [info]: acodec %s\n", dvd_trip.acodec);
		fprintf(stderr, "dvd_trip [info]: ovcopts %s\n", dvd_trip.vcodec_opts);
		fprintf(stderr, "dvd_trip [info]: oacopts %s\n", dvd_trip.acodec_opts);
		if(strlen(dvd_trip.vf_opts))
			fprintf(stderr, "dvd_trip [info]: vf %s\n", dvd_trip.vf_opts);
		fprintf(stderr, "dvd_trip [info]: output fps %s\n", dvd_trip.fps);
		if(dvd_trip.deinterlace)
			fprintf(stderr, "dvd_trip [info]: deinterlacing video\n");
	}

	mpv_initialize(dvd_mpv);
	mpv_command(dvd_mpv, dvd_mpv_commands);

	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN || dvd_mpv_event->event_id == MPV_EVENT_END_FILE)
			break;

		// Logging output
		if((verbose || debug) && dvd_mpv_event->event_id == MPV_EVENT_LOG_MESSAGE) {
			dvd_mpv_log_message = (struct mpv_event_log_message *)dvd_mpv_event->data;
			printf("mpv [%s]: %s", dvd_mpv_log_message->level, dvd_mpv_log_message->text);
		}

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
