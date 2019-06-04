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
#include "dvd_mpv.h"
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
	 */

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
	bool opt_force_encode = false;
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
	char dvd_mpv_first_chapter[4] = {'\0'};
	char dvd_mpv_last_chapter[4] = {'\0'};
	const char *home_dir = getenv("HOME");
	int retval = 0;

	// Video Title Set
	struct dvd_vts dvd_vts[99];

	struct dvd_trip dvd_trip;

	dvd_trip.track = 1;
	dvd_trip.first_chapter = 1;
	dvd_trip.last_chapter = 99;
	memset(dvd_trip.filename, '\0', sizeof(dvd_trip.filename));
	memset(dvd_trip.config_dir, '\0', sizeof(dvd_trip.config_dir));
	memset(dvd_trip.mpv_config_dir, '\0', sizeof(dvd_trip.mpv_config_dir));
	memset(dvd_trip.container, '\0', sizeof(dvd_trip.container));
	dvd_trip.encode_video = true;
	memset(dvd_trip.vcodec, '\0', sizeof(dvd_trip.vcodec));
	memset(dvd_trip.vcodec_opts, '\0', sizeof(dvd_trip.vcodec_opts));
	memset(dvd_trip.vcodec_log_level, '\0', sizeof(dvd_trip.vcodec_log_level));
	memset(dvd_trip.color_opts, '\0', sizeof(dvd_trip.color_opts));
	dvd_trip.encode_audio = true;
	memset(dvd_trip.acodec, '\0', sizeof(dvd_trip.acodec));
	memset(dvd_trip.acodec_opts, '\0', sizeof(dvd_trip.acodec_opts));
	memset(dvd_trip.audio_lang, '\0', sizeof(dvd_trip.audio_lang));
	memset(dvd_trip.audio_stream_id, '\0', sizeof(dvd_trip.audio_stream_id));
	dvd_trip.encode_subtitles = false;
	memset(dvd_trip.subtitles_lang, '\0', sizeof(dvd_trip.subtitles_lang));
	memset(dvd_trip.subtitles_stream_id, '\0', sizeof(dvd_trip.subtitles_stream_id));
	memset(dvd_trip.vf_opts, '\0', sizeof(dvd_trip.vf_opts));
	dvd_trip.crf = 22;
	memset(dvd_trip.fps, '\0', sizeof(dvd_trip.fps));
	dvd_trip.deinterlace = false;
	snprintf(dvd_trip.config_dir, PATH_MAX - 1, "/.config/dvd_trip");
	memset(dvd_trip.mpv_config_dir, '\0', sizeof(dvd_trip.mpv_config_dir));
	if(home_dir != NULL)
		snprintf(dvd_trip.mpv_config_dir, PATH_MAX - 1, "%s%s", home_dir, dvd_trip.config_dir);

	memset(dvd_mpv_first_chapter, '\0', sizeof(dvd_mpv_first_chapter));
	memset(dvd_mpv_last_chapter, '\0', sizeof(dvd_mpv_last_chapter));

	struct option long_options[] = {

		{ "audio", required_argument, 0, 'a' },
		{ "alang", required_argument, 0, 'D' },
		{ "aid", required_argument, 0, 'B' },

		{ "slang", required_argument, 0, 's' },
		{ "sid", required_argument, 0, 'S' },

		{ "chapters", required_argument, 0, 'c' },
		{ "track", required_argument, 0, 't' },
		{ "vid", required_argument, 0, 'E' },

		{ "deinterlace", no_argument, 0, 'd' },

		{ "output", required_argument, 0, 'o' },

		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },

		{ "force", no_argument, 0, 'f' },
		{ "verbose", no_argument, 0, 'v' },
		{ "debug", no_argument, 0, 'z' },
		{ 0, 0, 0, 0 }

	};

	while((opt = getopt_long(argc, argv, "AB:c:dD:E:fho:s:S:t:vz", long_options, &long_index )) != -1) {

		switch(opt) {

			case 'B':
				strncpy(dvd_trip.audio_stream_id, optarg, 3);
				if(strncmp(optarg, "no", 2) == 0)
					dvd_trip.encode_audio = false;
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

			case 'D':
				strncpy(dvd_trip.audio_lang, optarg, 2);
				break;

			case 'E':
				if(strncmp(optarg, "no", 2) == 0)
					dvd_trip.encode_video = false;
				break;

			case 'o':
				opt_filename = true;
				strncpy(dvd_trip.filename, optarg, PATH_MAX - 1);
				token_filename = strtok(optarg, ".");

				while(token_filename != NULL) {

					snprintf(tmp_filename, 5, "%s", token_filename);
					token_filename = strtok(NULL, ".");

					if(token_filename == NULL && strlen(tmp_filename) == 3 && strncmp(tmp_filename, "mkv", 3) == 0) {
						strcpy(dvd_trip.container, "mkv");
					} else if(token_filename == NULL && strlen(tmp_filename) == 3 && strncmp(tmp_filename, "mp4", 3) == 0) {
						strcpy(dvd_trip.container, "mp4");
					} else if(token_filename == NULL && strlen(tmp_filename) == 4 && strncmp(tmp_filename, "webm", 4) == 0) {
						strcpy(dvd_trip.container, "webm");
					}

				}

				break;

			case 's':
				strncpy(dvd_trip.subtitles_lang, optarg, 2);
				break;

			case 'S':
				strncpy(dvd_trip.subtitles_stream_id, optarg, 3);
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
				printf("      --alang <language>	Select audio language, two character code (default: first audio track)\n");
				printf("      --aid <#> 		Select audio track ID\n");
				printf("      --slang <language>	Select subtitles language, two character code (default: none)\n");
				printf("      --sid <#> 		Select subtitles track ID\n");
				printf("  -f, --force			Ignore invalid track warning\n");
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

	/** Begin dvd_trip :) */

	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "[dvd_trip] cannot access %s\n", device_filename);
		return 1;
	}

	// Check to see if device can be opened
	int dvd_fd = 0;
	dvd_fd = dvd_device_open(device_filename);
	if(dvd_fd < 0) {
		fprintf(stderr, "[dvd_trip] error opening %s\n", device_filename);
		return 1;
	}
	dvd_device_close(dvd_fd);


#ifdef __linux__

	// Poll drive status if it is hardware
	if(dvd_device_is_hardware(device_filename)) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "[dvd_trip] drive status: ");
			dvd_drive_display_status(device_filename);

			return 1;

		}

	}

#endif

	// begin libdvdread usage

	// Open DVD device
	dvd_reader_t *dvdread_dvd = NULL;
	dvdread_dvd = DVDOpen(device_filename);

	if(!dvdread_dvd) {
		fprintf(stderr, "[dvd_trip] Opening DVD %s failed\n", device_filename);
		return 1;
	}

	// Check if DVD has an identifier, fail otherwise
	char dvdread_id[DVD_DVDREAD_ID + 1];
	memset(dvdread_id, '\0', sizeof(dvdread_id));
	dvd_dvdread_id(dvdread_id, dvdread_dvd);
	if(strlen(dvdread_id) == 0) {
		fprintf(stderr, "[dvd_trip] Opening DVD %s failed\n", device_filename);
		return 1;
	}

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "[dvd_trip] Opening VMG IFO failed\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// DVD
	struct dvd_info dvd_info;
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	dvd_info.side = 1;
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	memset(dvd_info.provider_id, '\0', sizeof(dvd_info.provider_id));
	memset(dvd_info.vmg_id, '\0', sizeof(dvd_info.vmg_id));
	dvd_info.tracks = dvd_tracks(vmg_ifo);
	dvd_info.longest_track = 1;

	dvd_title(dvd_info.title, device_filename);
	printf("Disc title: '%s', ID: '%s', Num tracks: %" PRIu16 ", Longest track: %" PRIu16 "\n", dvd_info.title, dvdread_id, dvd_info.tracks, dvd_info.longest_track);

	uint16_t num_ifos = 1;
	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	if(num_ifos < 1) {
		fprintf(stderr, "[dvd_trip] DVD has no title IFOs\n");
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Cells
	struct dvd_cell dvd_cell;
	dvd_cell.cell = 1;
	memset(dvd_cell.length, '\0', sizeof(dvd_cell.length));
	snprintf(dvd_cell.length, DVD_CELL_LENGTH + 1, "00:00:00.000");
	dvd_cell.msecs = 0;

	// Open first IFO
	uint16_t vts = 1;
	ifo_handle_t *vts_ifo = NULL;

	// ???
	vts_ifo = ifoOpen(dvdread_dvd, vts);
	if(vts_ifo == NULL) {
		fprintf(stderr, "[dvd_trip] Could not open primary VTS_IFO\n");
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
	
	// Set the track number to the longest if none is passed as an argument
	if(!opt_track_number) {

		uint16_t ix = 0;
		uint16_t track = 1;
		uint32_t msecs = 0;
		uint32_t longest_msecs = 0;

		for(ix = 0, track = 1; ix < dvd_info.tracks; ix++, track++) {

			vts = dvd_vts_ifo_number(vmg_ifo, track);
			vts_ifo = vts_ifos[vts];

			msecs = dvd_track_msecs(vmg_ifo, vts_ifo, track);

			if(msecs > longest_msecs) {
				longest_msecs = msecs;
				dvd_trip.track = track;
			}

		}
	
	}

	// Track
	struct dvd_track dvd_track;
	dvd_track.track = dvd_trip.track;
	dvd_track.valid = true;
	dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, dvd_trip.track);
	vts_ifo = vts_ifos[dvd_track.vts];
	dvd_track.ttn = dvd_track_ttn(vmg_ifo, dvd_trip.track);
	dvd_track_length(dvd_track.length, vmg_ifo, vts_ifo, dvd_trip.track);
	dvd_track.msecs = dvd_track_msecs(vmg_ifo, vts_ifo, dvd_trip.track);
	dvd_track.chapters = dvd_track_chapters(vmg_ifo, vts_ifo, dvd_trip.track);
	dvd_track.audio_tracks = dvd_track_audio_tracks(vts_ifo);
	dvd_track.subtitles = dvd_track_subtitles(vts_ifo);
	dvd_track.active_audio_streams = dvd_audio_active_tracks(vmg_ifo, vts_ifo, dvd_trip.track);
	dvd_track.active_subs = dvd_track_active_subtitles(vmg_ifo, vts_ifo, dvd_trip.track);
	dvd_track.cells = dvd_track_cells(vmg_ifo, vts_ifo, dvd_trip.track);
	dvd_track.blocks = dvd_track_blocks(vmg_ifo, vts_ifo, dvd_trip.track);
	dvd_track.filesize = dvd_track_filesize(vmg_ifo, vts_ifo, dvd_trip.track);

	// Set the proper chapter range
	if(opt_chapter_number) {
		if(arg_first_chapter > dvd_track.chapters) {
			dvd_trip.first_chapter = dvd_track.chapters;
			fprintf(stderr, "[dvd_trip] resetting first chapter to %" PRIu8 "\n", dvd_trip.first_chapter);
		} else
			dvd_trip.first_chapter = arg_first_chapter;
		
		if(arg_last_chapter > dvd_track.chapters) {
			dvd_trip.last_chapter = dvd_track.chapters;
			fprintf(stderr, "[dvd_trip] resetting last chapter to %" PRIu8 "\n", dvd_trip.last_chapter);
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

	printf("Track: %02" PRIu16 ", Length: %s, Chapters: %02" PRIu8 ", Cells: %02" PRIu8 ", Audio streams: %02" PRIu8 ", Subpictures: %02" PRIu8 ", Blocks: %6zd, Filesize: %9zd\n", dvd_track.track, dvd_track.length, dvd_track.chapters, dvd_track.cells, dvd_track.audio_tracks, dvd_track.subtitles, dvd_track.blocks, dvd_track.filesize);

	// Check for track issues
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

	if(!dvd_track.valid) {

		fprintf(stderr, "[dvd_trip] track %" PRIu16 "has been flagged as invalid, and encoding it could cause strange problems\b", dvd_trip.track);

		if(!opt_force_encode) {

			fprintf(stderr, "[dvd_trip] pass --force to options to ignore warnings and continue encoding\b");

			DVDCloseFile(dvdread_vts_file);

			if(vts_ifo)
				ifoClose(vts_ifo);

			if(vmg_ifo)
				ifoClose(vmg_ifo);

			if(dvdread_dvd)
				DVDClose(dvdread_dvd);

			return 1;

		} else {

			fprintf(stderr, "[dvd_trip] forcing encode on invalid track %" PRIu16 "\n", dvd_trip.track);

		}

	}

	// Map the audio track passed as an argument to the audio stream ID
	// Selecting the audio track ID from dvd_info indexes is a long-term dream.
	// Unfortunately, it'd require a lot of work and testing to see if I can
	// match up what it parsed by the decoder, and if it matches the indexes I have
	// dvd_trip.audio_track = 0;

	/*
	bool opt_audio_track = false;
	char audio_ff_aid[12];
	memset(audio_ff_aid, '\0', sizeof(audio_ff_aid));
	if(dvd_trip.encode_audio && opt_audio_track) {

		// Get the audio stream ID
		char audio_stream_id[12] = {'\0'};
		uint8_t audio_tracks = dvd_track_audio_tracks(vts_ifo);
		uint8_t audio_track_ix = 0;
		bool audio_active = false;
		for(audio_track_ix = 0; audio_track_ix < audio_tracks + 1; audio_track_ix++) {

			audio_active = dvd_audio_active(vmg_ifo, vts_ifo, dvd_trip.track, audio_track_ix);
			if(audio_active && (dvd_trip.audio_track == audio_track_ix + 1)) {

				dvd_audio_stream_id(audio_stream_id, vts_ifo, audio_track_ix);
				strcpy(audio_ff_aid, audio_stream_id);

				break;

			}

		}

	}
	*/

	// MPV zero-indexes tracks
	sprintf(dvd_mpv_args, "dvdread://%" PRIu16, dvd_trip.track - 1);

	const char *dvd_mpv_commands[] = { "loadfile", dvd_mpv_args, NULL };

	// DVD playback using libmpv
	mpv_handle *dvd_mpv = NULL;
	dvd_mpv = mpv_create();

	if(dvd_mpv == NULL) {
		fprintf(stderr, "[dvd_trip] could not create an mpv instance\n");
		return 1;
	}

	// Load user's mpv configuration in ~/.config/dvd_trip/mpv.conf (and friends)
	if(strlen(dvd_trip.mpv_config_dir) > 0) {

		fprintf(stderr, "[dvd_trip] using mpv config dir: %s\n", dvd_trip.mpv_config_dir);
		retval = mpv_set_option_string(dvd_mpv, "config-dir", dvd_trip.mpv_config_dir);

		if(retval) {
			fprintf(stderr, "[dvd_trip] could not set mpv option 'config-dir' %s\n", dvd_trip.mpv_config_dir);
		}

		retval = mpv_set_option_string(dvd_mpv, "config", "yes");

		if(retval) {
			fprintf(stderr, "[dvd_trip] could not set mpv option 'config'\n");
		}

	}

	// Terminal output
	mpv_set_option_string(dvd_mpv, "terminal", "yes");

	// Debug output and progress bar flood output, making a big mess
	if(!debug) {
		mpv_set_option_string(dvd_mpv, "term-osd-bar", "yes");
		mpv_set_option_string(dvd_mpv, "term-osd-bar-chars", "[=+-]");
	}

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
	if(strncmp(dvd_trip.container, "mkv", 3) == 0 || !opt_filename) {

		if(!opt_filename)
			strcpy(dvd_trip.filename, "trip_encode.mkv");

		if(dvd_trip.encode_video) {
			strcpy(dvd_trip.vcodec, "libx265");
			sprintf(dvd_trip.vcodec_opts, "%s,preset=slow,crf=20,x265-params=log-level=%s", dvd_trip.color_opts, dvd_trip.vcodec_log_level);
		}

		if(dvd_trip.encode_audio) {
			strcpy(dvd_trip.acodec, "libfdk_aac,aac");
			strcpy(dvd_trip.acodec_opts, "b=192k");
		}

	}

	if(strncmp(dvd_trip.container, "mp4", 3) == 0) {

		if(!opt_filename)
			strcpy(dvd_trip.filename, "trip_encode.mp4");

		if(dvd_trip.encode_video) {
			strcpy(dvd_trip.vcodec, "libx264");
			// x264 doesn't allow passing log level (that I can see)
			sprintf(dvd_trip.vcodec_opts, "%s,preset=slow,crf=22", dvd_trip.color_opts);
		}

		if(dvd_trip.encode_audio) {
			strcpy(dvd_trip.acodec, "libfdk_aac,aac");
			strcpy(dvd_trip.acodec_opts, "b=192k");
		}

	}

	if(strncmp(dvd_trip.container, "webm", 4) == 0) {

		if(!opt_filename)
			strcpy(dvd_trip.filename, "trip_encode.webm");

		if(dvd_trip.encode_video) {
			strcpy(dvd_trip.vcodec, "libvpx-vp9");
			sprintf(dvd_trip.vcodec_opts, "%s,b=0,crf=22,keyint_min=0,g=360", dvd_trip.color_opts);
		}

		if(dvd_trip.encode_audio) {
			strcpy(dvd_trip.acodec, "libopus,opus,libvorbis,vorbis");
			strcpy(dvd_trip.acodec_opts, "application=audio");
			strcpy(dvd_trip.acodec_opts, "application=audio,b=192000");
		}

		/*
		 * keep these for reference because they were hard to decide on
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
		*/

	}

	mpv_set_option_string(dvd_mpv, "o", dvd_trip.filename);
	if(strlen(dvd_trip.vcodec) > 0)
		mpv_set_option_string(dvd_mpv, "ovc", dvd_trip.vcodec);
	if(strlen(dvd_trip.vcodec_opts) > 0)
		mpv_set_option_string(dvd_mpv, "ovcopts", dvd_trip.vcodec_opts);
	if(strlen(dvd_trip.acodec) > 0)
		mpv_set_option_string(dvd_mpv, "oac", dvd_trip.acodec);
	if(strlen(dvd_trip.acodec_opts) > 0)
		mpv_set_option_string(dvd_mpv, "oacopts", dvd_trip.acodec_opts);
	if(dvd_trip.encode_audio)
		mpv_set_option_string(dvd_mpv, "track-auto-selection", "yes");
	mpv_set_option_string(dvd_mpv, "dvd-device", device_filename);
	mpv_set_option_string(dvd_mpv, "input-default-bindings", "yes");
	mpv_set_option_string(dvd_mpv, "input-vo-keyboard", "yes");
	mpv_set_option_string(dvd_mpv, "resume-playback", "no");

	// MPV's chapter range starts at the first one, and ends at the last one plus one
	// fex: to play chapter 1 only, mpv --start '#1' --end '#2'
	sprintf(dvd_mpv_first_chapter, "#%" PRIu8, dvd_trip.first_chapter);
	sprintf(dvd_mpv_last_chapter, "#%" PRIu8, dvd_trip.last_chapter + 1);
	mpv_set_option_string(dvd_mpv, "start", dvd_mpv_first_chapter);
	mpv_set_option_string(dvd_mpv, "end", dvd_mpv_last_chapter);

	// User can pass "--vid no" to disable encoding any video
	if(!dvd_trip.encode_video)
		mpv_set_option_string(dvd_mpv, "vid", "no");

	// User can pass "--aid no" to disable encoding any sound
	if(strlen(dvd_trip.audio_lang) > 0)
		mpv_set_option_string(dvd_mpv, "alang", dvd_trip.audio_lang);
	else if(strlen(dvd_trip.audio_stream_id) > 0)
		mpv_set_option_string(dvd_mpv, "aid", dvd_trip.audio_stream_id);

	// Burn-in subtitles
	if(strlen(dvd_trip.subtitles_lang))
		mpv_set_option_string(dvd_mpv, "slang", dvd_trip.subtitles_lang);
	else if(strlen(dvd_trip.subtitles_stream_id) > 0)
		mpv_set_option_string(dvd_mpv, "sid", dvd_trip.subtitles_stream_id);

	// MPV will error out on its own if there's no video && no audio selected.
	// Don't quit out here though, because there could be other reasons MPV
	// fails to initialize -- let it process errors in its own way
	if(!dvd_trip.encode_video && !dvd_trip.encode_audio)
		fprintf(stderr, "[dvd_trip] no video or audio streams selected\n");

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

	fprintf(stderr, "[dvd_trip] [info]: dvd track %" PRIu16 "\n", dvd_trip.track);
	fprintf(stderr, "[dvd_trip] [info]: chapters %" PRIu8 " to %" PRIu8 "\n", dvd_trip.first_chapter, dvd_trip.last_chapter);
	fprintf(stderr, "[dvd_trip] [info]: saving to %s\n", dvd_trip.filename);
	if(dvd_trip.encode_video)
		fprintf(stderr, "[dvd_trip] [info]: vcodec %s\n", dvd_trip.vcodec);
	if(dvd_trip.encode_audio)
		fprintf(stderr, "[dvd_trip] [info]: acodec %s\n", dvd_trip.acodec);
	if(dvd_trip.encode_video)
		fprintf(stderr, "[dvd_trip] [info]: ovcopts %s\n", dvd_trip.vcodec_opts);
	if(dvd_trip.encode_audio)
		fprintf(stderr, "[dvd_trip] [info]: oacopts %s\n", dvd_trip.acodec_opts);
	if(strlen(dvd_trip.vf_opts))
		fprintf(stderr, "dvd_trip [info]: vf %s\n", dvd_trip.vf_opts);
	if(strlen(dvd_trip.subtitles_lang))
		fprintf(stderr, "dvd_trip [info]: burn-in subtitles lang %s\n", dvd_trip.subtitles_lang);
	else if(strlen(dvd_trip.subtitles_stream_id))
		fprintf(stderr, "dvd_trip [info]: burn-in subtitles stream %s\n", dvd_trip.subtitles_stream_id);
	fprintf(stderr, "[dvd_trip] [info]: output fps %s\n", dvd_trip.fps);
	if(dvd_trip.deinterlace && dvd_trip.encode_video)
		fprintf(stderr, "[dvd_trip] [info]: deinterlacing video\n");

	fprintf(stderr, "[dvd_trip] [info]: mpv: %s\n", dvd_mpv_args);

	// ** All encoding options must be set before initialize **
	retval = mpv_initialize(dvd_mpv);

	if(retval) {
		fprintf(stderr, "[dvd_trip] mpv_initialize() failed\n");
		return 1;
	}

	retval = mpv_command(dvd_mpv, dvd_mpv_commands);

	if(retval) {
		fprintf(stderr, "[dvd_trip] mpv_command() failed\n");
		return 1;
	}

	mpv_event *dvd_mpv_event = NULL;
	struct mpv_event_log_message *dvd_mpv_log_message = NULL;

	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN || dvd_mpv_event->event_id == MPV_EVENT_END_FILE)
			break;

		// Logging output
		if((verbose || debug) && dvd_mpv_event->event_id == MPV_EVENT_LOG_MESSAGE) {
			dvd_mpv_log_message = (struct mpv_event_log_message *)dvd_mpv_event->data;
			printf("[libmpv] [%s]: %s", dvd_mpv_log_message->level, dvd_mpv_log_message->text);
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
