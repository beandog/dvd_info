#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
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
#include "config.h"
#include "dvd_info.h"
#include "dvd_device.h"
#include "dvd_drive.h"
#include "dvd_open.h"
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

	/**
	 *      _          _          _
	 *   __| |_   ____| |    _ __(_)_ __
	 *  / _` \ \ / / _` |   | '__| | '_ \
	 * | (_| |\ V / (_| |   | |  | | |_) |
 	 *  \__,_| \_/ \__,_|___|_|  |_| .__/
         *                 |_____|     |_|
	 *
	 * ** the tiniest little dvd ripper you'll ever see :) **
	 *
	 * About
	 *
	 * dvd_rip is designed to be a tiny DVD ripper with very few options. The purpose being
	 * that there is as little overhead as possible when wanting to do a quick DVD rip, and
	 * has a good set of default encoding settings.
	 *
	 * Just running "dvd_rip" alone with no arguments or options will fetch the longest track
	 * on the DVD, select the first English audio track, and encode it to H.264 using libx264 with
	 * its default preset and CRF, and AAC audio.
	 *
	 * Because it is so tiny, some options it doesn't have that larger DVD ripper applications
	 * would are: encoding multiple audio streams, audio passthrough, subtitle support (VOBSUB and
	 * closed captioning), auto-cropping black bars, and custom audio and video codec options.
	 *
	 * However(!), if you want more features when ripping the DVD, you can use "dvd_copy" instead,
	 * and output the track to stdout. Using ffmpeg can detect the streams and encode, crop,
	 * copy, and so on directly. Example: dvd_copy -o - | ffprobe -i -
	 *
	 * Presets
	 *
	 * To keep things simple, dvd_rip only has three presets which fit the most commonly used
	 * codecs and formats when ripping DVDs: "mp4", "mkv", "webm".
	 *
	 */

int main(int argc, char **argv) {

	/**
	 * Parse options
	 */

	bool verbose = false;
	bool debug = false;
	char device_filename[PATH_MAX];
	bool invalid_opts = false;
	bool opt_track_number = false;
	bool opt_chapter_number = false;
	bool opt_filename = false;
	bool x264 = false;
	bool x265 = false;
	bool vp8 = false;
	bool vp9 = false;
	bool opus = false;
	bool aac = false;
	bool mkv = false;
	bool mp4 = false;
	bool webm = false;
	bool detelecine = true;
	bool pal_video = false;
	int8_t crf = -1;
	char str_crf[10];
	uint16_t arg_track_number = 1;
	int long_index = 0;
	int opt = 0;
	unsigned long int arg_number = 0;
	uint8_t arg_first_chapter = 1;
	uint8_t arg_last_chapter = 99;
	char *token = NULL;
	char dvd_mpv_first_chapter[4] = {'\0'};
	char dvd_mpv_last_chapter[4] = {'\0'};
	const char *home_dir = getenv("HOME");
	char dvd_mpv_args[64];
	int retval = 0;

	// Video Title Set
	struct dvd_vts dvd_vts[99];

	struct dvd_rip dvd_rip;

	dvd_rip.track = 1;
	dvd_rip.first_chapter = 1;
	dvd_rip.last_chapter = 99;
	memset(dvd_rip.filename, '\0', PATH_MAX);
	memset(dvd_rip.config_dir, '\0', PATH_MAX);
	memset(dvd_rip.mpv_config_dir, '\0', PATH_MAX);
	memset(dvd_rip.container, '\0', sizeof(dvd_rip.container));
	memset(dvd_rip.vcodec, '\0', sizeof(dvd_rip.vcodec));
	memset(dvd_rip.vcodec_opts, '\0', sizeof(dvd_rip.vcodec_opts));
	memset(dvd_rip.vcodec_log_level, '\0', sizeof(dvd_rip.vcodec_log_level));
	memset(dvd_rip.acodec, '\0', sizeof(dvd_rip.acodec));
	memset(dvd_rip.acodec_opts, '\0', sizeof(dvd_rip.acodec_opts));
	memset(dvd_rip.audio_lang, '\0', sizeof(dvd_rip.audio_lang));
	memset(dvd_rip.audio_stream_id, '\0', sizeof(dvd_rip.audio_stream_id));
	dvd_rip.audio_bitrate = 0;
	dvd_rip.encode_subtitles = false;
	memset(dvd_rip.subtitles_lang, '\0', sizeof(dvd_rip.subtitles_lang));
	memset(dvd_rip.subtitles_stream_id, '\0', sizeof(dvd_rip.subtitles_stream_id));
	memset(dvd_rip.vf_opts, '\0', sizeof(dvd_rip.vf_opts));
	memset(dvd_rip.of_opts, '\0', sizeof(dvd_rip.of_opts));
	memset(str_crf, '\0', sizeof(str_crf));
	snprintf(dvd_rip.config_dir, PATH_MAX - 1, "/.config/dvd_rip");
	memset(dvd_mpv_first_chapter, '\0', sizeof(dvd_mpv_first_chapter));
	memset(dvd_mpv_last_chapter, '\0', sizeof(dvd_mpv_last_chapter));
	memset(dvd_mpv_args, '\0', sizeof(dvd_mpv_args));

	if(home_dir != NULL)
		snprintf(dvd_rip.mpv_config_dir, PATH_MAX - 1, "%s%s", home_dir, dvd_rip.config_dir);

	struct option long_options[] = {

		{ "audio", required_argument, 0, 'a' },
		{ "alang", required_argument, 0, 'L' },
		{ "aid", required_argument, 0, 'B' },

		{ "slang", required_argument, 0, 's' },
		{ "sid", required_argument, 0, 'S' },

		{ "track", required_argument, 0, 't' },
		{ "chapters", required_argument, 0, 'c' },

		{ "output", required_argument, 0, 'o' },

		{ "no-detelecine", no_argument, 0, 'D' },

		{ "vcodec", required_argument, 0, 'v'},
		{ "acodec", required_argument, 0, 'a'},
		{ "crf", required_argument, 0, 'q' },

		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },

		{ "verbose", no_argument, 0, 'x' },
		{ "debug", no_argument, 0, 'z' },
		{ 0, 0, 0, 0 }

	};

	while((opt = getopt_long(argc, argv, "a:B:c:DhL:o:q:s:S:t:Vv:xz", long_options, &long_index )) != -1) {

		switch(opt) {

			case 'a':
				if(strncmp(optarg, "aac", 3) == 0) {
					aac = true;
				} else if(strncmp(optarg, "opus", 4) == 0) {
					opus = true;
				}
				break;

			case 'B':
				strncpy(dvd_rip.audio_stream_id, optarg, 3);
				break;

			case 'c':
				opt_chapter_number = true;
				token = strtok(optarg, "-");
				if(strlen(token) > 2) {
					fprintf(stderr, "[dvd_rip] Chapter range must be between 1 and 99\n");
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
						fprintf(stderr, "[dvd_rip] Chapter range must be between 1 and 99\n");
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

				if(arg_last_chapter < arg_first_chapter) {
					fprintf(stderr, "[dvd_rip] Last chapter must be a greater number than first chapter\n");
					return 1;
				}

				if(arg_first_chapter > arg_last_chapter) {
					fprintf(stderr, "[dvd_rip] First chapter must be a lower number than first chapter\n");
					return 1;
				}

				break;

			case 'D':
				detelecine = false;
				break;

			case 'L':
				strncpy(dvd_rip.audio_lang, optarg, 2);
				break;

			case 'o':
				opt_filename = true;
				strncpy(dvd_rip.filename, optarg, PATH_MAX - 1);

				if(strcmp(strrchr(optarg, '\0') - 4, ".mkv") == 0) {
					mkv = true;
				} else if(strcmp(strrchr(optarg, '\0') - 5, ".webm") == 0) {
					webm = true;
				} else if(strcmp(strrchr(optarg, '\0') - 4, ".mp4") == 0) {
					mp4 = true;
				}
				break;

			case 'q':
				arg_number = strtoul(optarg, NULL, 10);
				if(arg_number > 63)
					arg_number = 63;
				crf = (int8_t)arg_number;
				break;

			case 's':
				strncpy(dvd_rip.subtitles_lang, optarg, 2);
				break;

			case 'S':
				strncpy(dvd_rip.subtitles_stream_id, optarg, 3);
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
				printf("dvd_rip %s\n", PACKAGE_VERSION);
				return 0;

			case 'x':
				verbose = true;
				break;

			case 'v':
				if(strncmp(optarg, "x264", 4) == 0) {
					x264 = true;
				} else if(strncmp(optarg, "x265", 4) == 0) {
					x265 = true;
				} else if(strncmp(optarg, "vp8", 3) == 0) {
					vp8 = true;
				} else if(strncmp(optarg, "vp9", 3) == 0) {
					vp9 = true;
				}
				break;


			case 'z':
				verbose = true;
				debug = true;
				break;

			case '?':
				invalid_opts = true;
			case 'h':
				printf("dvd_rip - a tiny DVD ripper\n");
				printf("\n");
				printf("Usage:\n");
				printf("  dvd_rip [path] [options]\n");
				printf("\n");
				printf("  -o, --output <filename>       Save to filename (default: dvd_track_##.mp4\n");
				printf("\n");
				printf("Track selection:\n");
				printf("  -t, --track <#>          	Encode selected track (default: longest)\n");
				printf("  -c, --chapter <#>[-#]         Encode chapter number or range (default: all)\n");
				printf("\n");
				printf("Language selection:\n");
				printf("  --alang <language>            Select audio language, two character code (default: first audio track)\n");
				printf("  --aid <#>                     Select audio track ID\n");
				printf("  --slang <language>            Select subtitles language, two character code (default: none)\n");
				printf("  --sid <#>                     Select subtitles track ID\n");
				printf("\n");
				printf("Encoding options:\n");
				printf("\n");
				printf("  -v, --vcodec <vcodec>         Video codec <x264|x265|vp8|vp9>, default: x264\n");
				printf("  -a, --acodec <acodec>         Audio codec (aac|opus), default: aac\n");
				printf("  -q, --crf <#>			Video encoder CRF (x264 and x265)\n");
				printf("  -D, --no-detelecine           Do not detelecine video\n");
				printf("\n");
				printf("Defaults:\n");
				printf("\n");
				printf("By default, dvd_rip will encode source to H.264 video with AAC audio in an\n");
				printf("MP4 container. If an output filename is given with a different extension,\n");
				printf("it will use the default settings. for those instead. In each case, the default\n");
				printf("presets are used as selected by the codecs as well. Note that mpv must already\n");
				printf("be built with support for these codecs, or dvd_rip will quit.\n\n");
				printf("See the man page for more details.\n");
				printf("\n");
				printf("Other:\n");
				printf("  --verbose                     Show verbose output\n");
				printf("  --debug                       Show debugging output\n");
				printf("  --version                     Show version info and exit\n");
				printf("  -h, --help                    Show this help text and exit\n");
				printf("\n");
				printf("DVD path can be a device name, a single file, or directory (default: %s)\n", DEFAULT_DVD_DEVICE);
				printf("\n");
				printf("dvd_rip reads a configuration file from ~/.config/dvd_rip/mpv.conf\n");
				printf("\n");
				printf("See mpv man page for syntax and dvd_rip man page for examples.\n");
				if(invalid_opts)
					return 1;
				return 0;

			// let getopt_long set the variable
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

	/** Begin dvd_rip :) */

	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "[dvd_rip] cannot access %s\n", device_filename);
		return 1;
	}

#ifdef __linux__
	// Check for device drive status
	retval = device_open(device_filename);
	if(retval == 1)
		return 1;
#endif

	// begin libdvdread usage

	// Open DVD device
	dvd_reader_t *dvdread_dvd = NULL;
	dvdread_dvd = DVDOpen(device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "[dvd_rip] Opening DVD %s failed\n", device_filename);
		return 1;
	}

	// Check if DVD has an identifier, fail otherwise
	char dvdread_id[DVD_DVDREAD_ID + 1];
	memset(dvdread_id, '\0', sizeof(dvdread_id));
	dvd_dvdread_id(dvdread_id, dvdread_dvd);
	if(strlen(dvdread_id) == 0) {
		fprintf(stderr, "[dvd_rip] Opening DVD %s failed\n", device_filename);
		return 1;
	}

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "[dvd_rip] Opening VMG IFO failed\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// DVD
	struct dvd_info dvd_info;
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	dvd_info.tracks = dvd_tracks(vmg_ifo);
	dvd_info.longest_track = 1;
	dvd_title(dvd_info.title, device_filename);

	uint16_t num_ifos = 1;
	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	if(num_ifos < 1) {
		fprintf(stderr, "[dvd_rip] DVD has no title IFOs\n");
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

	vts_ifo = ifoOpen(dvdread_dvd, vts);
	if(vts_ifo == NULL) {
		fprintf(stderr, "[dvd_rip] Could not open primary VTS_IFO\n");
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
		fprintf(stderr, "[dvd_rip] Invalid track number %" PRIu16 "\n", arg_track_number);
		fprintf(stderr, "[dvd_rip] Valid track numbers: 1 to %" PRIu16 "\n", dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	} else if(opt_track_number) {
		dvd_rip.track = arg_track_number;
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
				dvd_rip.track = track;
			}

		}

	}

	// Track
	struct dvd_track dvd_track;
	dvd_track.track = dvd_rip.track;
	dvd_track.valid = true;
	dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, dvd_rip.track);
	vts_ifo = vts_ifos[dvd_track.vts];
	dvd_track.ttn = dvd_track_ttn(vmg_ifo, dvd_rip.track);
	dvd_track_length(dvd_track.length, vmg_ifo, vts_ifo, dvd_rip.track);
	dvd_track.msecs = dvd_track_msecs(vmg_ifo, vts_ifo, dvd_rip.track);
	dvd_track.chapters = dvd_track_chapters(vmg_ifo, vts_ifo, dvd_rip.track);
	dvd_track.audio_tracks = dvd_track_audio_tracks(vts_ifo);
	dvd_track.subtitles = dvd_track_subtitles(vts_ifo);
	dvd_track.active_audio_streams = dvd_audio_active_tracks(vmg_ifo, vts_ifo, dvd_rip.track);
	dvd_track.active_subs = dvd_track_active_subtitles(vmg_ifo, vts_ifo, dvd_rip.track);
	dvd_track.cells = dvd_track_cells(vmg_ifo, vts_ifo, dvd_rip.track);
	dvd_track.blocks = dvd_track_blocks(vmg_ifo, vts_ifo, dvd_rip.track);
	dvd_track.filesize = dvd_track_filesize(vmg_ifo, vts_ifo, dvd_rip.track);

	// Calculating the number of output chapters and bitrate
	// It's a cheap workaround, but to guarantee that for lossless audio codecs
	// all the channels are included. Since I can't accurately map ffmpeg, mpv and dvd_info
	// audio indexes, simply loop through all of them and see what the highest value is.
	uint8_t audio_track_ix;
	uint8_t audio_max_channels = 1;
	uint8_t audio_track_channels = 1;
	for(audio_track_ix = 0; audio_track_ix < dvd_track.audio_tracks; audio_track_ix++) {

		audio_track_channels = dvd_audio_channels(vts_ifo, audio_track_ix);

		if(audio_track_channels > audio_max_channels)
			audio_max_channels = audio_track_channels;

	}

	// Choose audio bitrates of DVDs based on num. of channels for Dolby Digital
	// I've seen source audio bitrates of 384 to 448 for 4, 5, and 6 channels. Use
	// 448k if higher than 3. Used as an array in case I change my mind in the future.
	// I haven't found any DVDs with 3 channels of audio on valid tracks.
	uint32_t dvd_bitrate[7] = { 0, 192, 192, 384, 448, 448, 448 };
	dvd_rip.audio_bitrate = dvd_bitrate[audio_max_channels];

	if(debug)
		fprintf(stderr, "[dvd_rip] setting max audio channels to %" PRIu8 "\n", audio_max_channels);

	// Set the proper chapter range
	if(opt_chapter_number) {
		if(arg_first_chapter > dvd_track.chapters) {
			fprintf(stderr, "[dvd_rip] Starting chapter cannot be larger than %" PRIu8 "\n", dvd_track.chapters);
			return 1;
		} else
			dvd_rip.first_chapter = arg_first_chapter;

		if(arg_last_chapter > dvd_track.chapters) {
			fprintf(stderr, "[dvd_rip] Final chapter cannot be larger than %" PRIu8 "\n", dvd_track.chapters);
			return 1;
		} else
			dvd_rip.last_chapter = arg_last_chapter;
	} else {
		dvd_rip.first_chapter = 1;
		dvd_rip.last_chapter = dvd_track.chapters;
	}

	/**
	 * File descriptors and filenames
	 */
	dvd_file_t *dvdread_vts_file = NULL;

	vts = dvd_vts_ifo_number(vmg_ifo, dvd_rip.track);
	vts_ifo = vts_ifos[vts];

	// Open the VTS VOB
	dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_TITLE_VOBS);

	if(strlen(dvd_info.title))
		fprintf(stderr, "[dvd_rip] Disc title: '%s'\n", dvd_info.title);
	if(strlen(dvdread_id))
		fprintf(stderr, "[dvd_rip] Disc ID: '%s'\n", dvdread_id);
	fprintf(stderr, "[dvd_rip] Track: %*" PRIu16 ", Length: %s, Chapters: %*" PRIu8 ", Cells: %*" PRIu8 ", Audio streams: %*" PRIu8 ", Subpictures: %*" PRIu8 "\n", 2, dvd_track.track, dvd_track.length, 2, dvd_track.chapters, 2, dvd_track.cells, 2, dvd_track.audio_tracks, 2, dvd_track.subtitles);

	// Check for track issues
	if(dvd_vts[vts].valid == false) {
		dvd_track.valid = false;
	}

	if(dvd_track.msecs == 0) {
		fprintf(stderr, "[dvd_rip] invalid track - track has zero length\n");
		dvd_track.valid = false;
	}

	if(dvd_track.chapters == 0) {
		fprintf(stderr, "[dvd_rip] invalid track - zero chapters\n");
		dvd_track.valid = false;
	}

	if(dvd_track.cells == 0) {
		fprintf(stderr, "[dvd_rip] invalid track - zero cells\n");
		dvd_track.valid = false;
	}

	if(!dvd_track.valid) {
		fprintf(stderr, "[dvd_rip] track %" PRIu16 "has been flagged as invalid, and encoding it could cause strange problems\b", dvd_rip.track);
	}

	// MPV instance
	mpv_handle *dvd_mpv = NULL;
	dvd_mpv = mpv_create();
	if(dvd_mpv == NULL) {
		fprintf(stderr, "[dvd_rip] could not create an mpv instance\n");
		return 1;
	}
	mpv_set_option_string(dvd_mpv, "config-dir", dvd_rip.mpv_config_dir);
	mpv_set_option_string(dvd_mpv, "config", "yes");
	mpv_set_option_string(dvd_mpv, "terminal", "yes");
	if(debug) {
		mpv_request_log_messages(dvd_mpv, "debug");
		strcpy(dvd_rip.vcodec_log_level, "full");
	} else if (verbose) {
		mpv_request_log_messages(dvd_mpv, "v");
		strcpy(dvd_rip.vcodec_log_level, "info");
	} else {
		mpv_request_log_messages(dvd_mpv, "info");
		strcpy(dvd_rip.vcodec_log_level, "info");
	}

	/** DVD **/
	snprintf(dvd_mpv_args, sizeof(dvd_mpv_args), "dvd://%" PRIu16, dvd_rip.track - 1);
	const char *dvd_mpv_commands[] = { "loadfile", dvd_mpv_args, NULL };
	mpv_set_option_string(dvd_mpv, "dvd-device", device_filename);

	/// Chapters
	snprintf(dvd_mpv_first_chapter, sizeof(dvd_mpv_first_chapter), "#%" PRIu8, dvd_rip.first_chapter);
	mpv_set_option_string(dvd_mpv, "start", dvd_mpv_first_chapter);

	if(dvd_rip.last_chapter == dvd_rip.first_chapter && dvd_rip.last_chapter < dvd_track.chapters)
		dvd_rip.last_chapter += 1;
	snprintf(dvd_mpv_last_chapter, sizeof(dvd_mpv_last_chapter), "#%" PRIu8, dvd_rip.last_chapter);
	mpv_set_option_string(dvd_mpv, "end", dvd_mpv_last_chapter);

	// Default container if using specific codecs
	if(!mp4 && !mkv && !webm && (x264 || x265))
		mp4 = true;
	if(!mp4 && !mkv && !webm && (vp8 || vp9))
		webm = true;

	// Default container MP4
	if(!mp4 && !mkv && !webm)
		mp4 = true;

	// Default video codec for MP4 and MKV
	if(!x264 && !x265 && !vp8 && !vp9)
		x264 = true;

	// Default audio codec for WebM
	if(webm && !aac && !opus)
		opus = true;

	// Default audio codec for MP4
	if(mp4 && !aac && !opus)
		aac = true;

	// Default audio codec for MKV
	if(mkv && !aac && !opus)
		aac = true;

	// Default video codec for WebM
	if(webm && !vp8 && !vp9) {
		x264 = false;
		vp8 = true;
	}

	// Default audio codec for WebM
	if(webm && !aac && !opus)
		opus = true;

	// MP4 can't support VP8 or VP9
	if(mp4 && (vp8 || vp9)) {
		fprintf(stderr, "[dvd_rip] MP4 doesn't support VP8 or VP9 video codecs, quitting\n");
		return 1;
	}

	// WebM can't support x264 or x265
	if(webm && (x264 || x265)) {
		fprintf(stderr, "[dvd_rip] WebM only supports VP8 and VP9 video codecs, quitting\n");
		return 1;
	}

	// WebM can't support AAC
	if(webm && aac) {
		fprintf(stderr, "[dvd_rip] WebM only supports Opus audio codec, quitting\n");
		return 1;
	}

	/** Filename **/
	if(!opt_filename) {
		if(mp4)
			strcpy(dvd_rip.container, "mp4");
		else if(mkv)
			strcpy(dvd_rip.container, "mkv");
		else if(webm)
			strcpy(dvd_rip.container, "webm");
		sprintf(dvd_rip.filename, "dvd_track_%02" PRIu16 ".%s", dvd_rip.track, dvd_rip.container);
	}

	mpv_set_option_string(dvd_mpv, "o", dvd_rip.filename);

	fprintf(stderr, "[dvd_rip] saving to filename \'%s\'\n", dvd_rip.filename);

	/** Video **/

	// Fix input CRF if needed
	if((x264 || x265) && crf > 51)
		crf = 51;
	if((vp8 || vp9) && crf > 63)
		crf = 63;
	if(x264 && crf == -1)
		crf = 20;
	if(x265 && crf == -1)
		crf = 22;
	if(crf > -1 && (x264 || x265)) {
		snprintf(dvd_rip.vcodec_opts, 9, "crf=%i", crf);
	}

	if(vp8 || vp9) {

		/* Trying to set encoding parameters for VPX is a pain, because I can't
		 * tell if I've got the syntax wrong or not, though everything from ffmpeg
		 * docs looks like I'm doing it right. That being said, setting a CRF
		 * does not work, so I'm trying to set a relevantly decent bitrate for
		 * both vp8 and vp9.
		 *
		 * I would like to use qmin and qmax as my second choice, but that's not
		 * working either. Using ffmpeg docs as reference:
		 * https://trac.ffmpeg.org/wiki/Encode/VP8
		 * https://ffmpeg.org//ffmpeg-all.html#libvpx
		 *
		 * Here's all the things I've tested that *don't* work:
		 * - crf=20,b=0
		 * - qmin=0,qmax=50
		 * - qmin=0,qmax=50,crf=20
		 *
		 * Another issue is that libvpx encodes using *one* processor at a time,
		 * so I'm overriding it here to actual # available (see sysinfo.h). It's
		 * obviously much faster now.
		 *
		 * So here you go, me guessing at bitrates. Hopefully in the future
		 * I'll figure this out, but for now, this is it.
		 */

		if(vp8)
			sprintf(dvd_rip.vcodec_opts, "b=1500k,cpu-used=%d", get_nprocs());
		else
			sprintf(dvd_rip.vcodec_opts, "b=1200k,cpu-used=%d", get_nprocs());

	}

	// Video codecs and encoding options
	if (x264)
		strcpy(dvd_rip.vcodec, "libx264");
	else if(x265)
		strcpy(dvd_rip.vcodec, "libx265");
	else if (vp8) {
		strcpy(dvd_rip.vcodec, "libvpx");
	} else if (vp9) {
		strcpy(dvd_rip.vcodec, "libvpx-vp9");
	}

	mpv_set_option_string(dvd_mpv, "ovc", dvd_rip.vcodec);

	// Containers
	// FIXME, this may need to be changed to movflags=+faststart, see mpv's
	// etc/encoding-profiles.conf.
	// Also see https://ffmpeg.org/ffmpeg-all.html#mov_002c-mp4_002c-ismv
	// for setting it in ffmpeg. Needs testing.
	if(mp4)
		strcpy(dvd_rip.of_opts, "movflags=empty_moov");

	if(strlen(dvd_rip.vcodec_opts))
		mpv_set_option_string(dvd_mpv, "ovcopts", dvd_rip.vcodec_opts);

	if(strlen(dvd_rip.of_opts))
		mpv_set_option_string(dvd_mpv, "ofopts", "movflags=empty_moov");

	// Detelecining
	if(detelecine && pal_video)
		strcat(dvd_rip.vf_opts, "pullup,dejudder,fps=25");
	else if(detelecine && !pal_video)
		strcat(dvd_rip.vf_opts, "pullup,dejudder,fps=fps=30000/1001");

	mpv_set_option_string(dvd_mpv, "vf", dvd_rip.vf_opts);

	/** Audio **/
	if(strlen(dvd_rip.audio_lang))
		mpv_set_option_string(dvd_mpv, "alang", dvd_rip.audio_lang);
	else if(strlen(dvd_rip.audio_stream_id))
		mpv_set_option_string(dvd_mpv, "aid", dvd_rip.audio_stream_id);

	// Audio codec
	if(aac)
		strcpy(dvd_rip.acodec, "aac");
	else if(opus)
		strcpy(dvd_rip.acodec, "libopus");

	mpv_set_option_string(dvd_mpv, "oac", dvd_rip.acodec);

	// Use a high bitrate by default to guarantee good sound quality
	mpv_set_option_string(dvd_mpv, "oacopts", "b=256k");

	/** Subtitles **/
	if(strlen(dvd_rip.subtitles_lang)) {
		fprintf(stderr, "[dvd_rip] burn-in subtitles lang %s\n", dvd_rip.subtitles_lang);
		mpv_set_option_string(dvd_mpv, "slang", dvd_rip.subtitles_lang);
	} else if(strlen(dvd_rip.subtitles_stream_id)) {
		fprintf(stderr, "[dvd_rip] burn-in subtitles stream %s\n", dvd_rip.subtitles_stream_id);
		mpv_set_option_string(dvd_mpv, "sid", dvd_rip.subtitles_stream_id);
	}

	// ** All encoding and user config options must be set before initialize **
	retval = mpv_initialize(dvd_mpv);
	if(retval) {
		fprintf(stderr, "[dvd_rip] mpv_initialize() failed\n");
		return 1;
	}

	retval = mpv_command(dvd_mpv, dvd_mpv_commands);
	if(retval) {
		fprintf(stderr, "[dvd_rip] mpv_command() failed\n");
		return 1;
	}

	// Print what settings are used at this point
	/*
	fprintf(stderr, "[libmpv] ovc = %s\n", mpv_get_property_string(dvd_mpv, "ovc"));
	fprintf(stderr, "[libmpv] ovcopts = %s\n", mpv_get_property_string(dvd_mpv, "ovcopts"));
	if(strlen(dvd_rip.vf_opts))
		fprintf(stderr, "[libmpv] vf = %s\n", mpv_get_property_string(dvd_mpv, "vf"));
	if(strlen(dvd_rip.audio_lang))
		fprintf(stderr, "[libmpv] alang = %s\n", mpv_get_property_string(dvd_mpv, "alang"));
	else if(strlen(dvd_rip.audio_stream_id))
		fprintf(stderr, "[libmpv] aid = %s\n", mpv_get_property_string(dvd_mpv, "aid"));
	fprintf(stderr, "[libmpv] acodec = %s\n", mpv_get_property_string(dvd_mpv, "oac"));
	printf("[dvd_rip] mpv dvd://%" PRIu16 " # mpv zero-indexes tracks\n", dvd_rip.track - 1);
	fprintf(stderr, "[libmpv] slang = %s\n", mpv_get_property_string(dvd_mpv, "slang"));
	fprintf(stderr, "[libmpv] sid = %s\n", mpv_get_property_string(dvd_mpv, "sid"));
	*/

	mpv_set_option_string(dvd_mpv, "sid", "none");

	mpv_event *dvd_mpv_event = NULL;
	struct mpv_event_log_message *dvd_mpv_log_message = NULL;
	struct mpv_event_end_file *dvd_mpv_eof = NULL;

	retval = 0;

	if(x264 || x265)
		fprintf(stderr, "[dvd_rip] using video codec %s and CRF %i\n", dvd_rip.vcodec, crf);
	if(vp8 || vp9)
		fprintf(stderr, "[dvd_rip] using video codec %s\n", dvd_rip.vcodec);
	fprintf(stderr, "[dvd_rip] using audio codec %s\n", dvd_rip.acodec);
	if(detelecine)
		printf("[dvd_rip] detelecining video using pullup, dejudder, fps filters\n");

	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN)
			break;

		if(dvd_mpv_event->event_id == MPV_EVENT_END_FILE) {

			dvd_mpv_eof = dvd_mpv_event->data;

			if(dvd_mpv_eof->reason == MPV_END_FILE_REASON_QUIT) {

				fprintf(stderr, "[dvd_rip] stopping encode\n");
				break;

			}

			if(dvd_mpv_eof->reason == MPV_END_FILE_REASON_ERROR) {

				retval = 1;
				fprintf(stderr, "[dvd_rip] libmpv hit end of file error, video may have problems at end\n");

				if(dvd_mpv_eof->error == MPV_ERROR_NOTHING_TO_PLAY) {
					fprintf(stderr, "[dvd_rip] no audio or video data to play\n");
				}

			}

			break;

		}

		// Logging output
		if((verbose || debug) && dvd_mpv_event->event_id == MPV_EVENT_LOG_MESSAGE) {
			dvd_mpv_log_message = (struct mpv_event_log_message *)dvd_mpv_event->data;
			printf("[dvd_rip] mpv event log message - %s", dvd_mpv_log_message->text);
		}

	}

	mpv_terminate_destroy(dvd_mpv);

	fprintf(stderr, "[dvd_rip] encode finished\n");

	if(retval == 0) {
		fprintf(stderr, "[dvd_rip] file saved to '%s'\n", dvd_rip.filename);
	} else {
		fprintf(stderr, "[dvd_rip] encoding errors, file may be incomplete: '%s'\n", dvd_rip.filename);
	}

	DVDCloseFile(dvdread_vts_file);

	if(vts_ifo)
		ifoClose(vts_ifo);

	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	return 0;

}
