#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#ifdef __linux__
#include <linux/cdrom.h>
#include <linux/limits.h>
#include <sys/sysinfo.h>
#include "dvd_drive.h"
#else
#include <limits.h>
#endif
#if defined (__FreeBSD__)
#include <sys/sysctl.h>
#endif
#if defined (__MINGW32__) || defined (__CYGWIN__) || defined (__MSYS__)
#include <sysinfoapi.h>
#endif
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_config.h"
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
	 *      _          _    _        _
	 *   __| |_   ____| |  | |_ _ __(_)_ __
	 *  / _` \ \ / / _` |  | __| '__| | '_ \
	 * | (_| |\ V / (_| |  | |_| |  | | |_) |
	 *  \__,_| \_/ \__,_|___\__|_|  |_| .__/
	 *                 |_____|        |_|
	 *
	 * dvd_trip is designed to be a tiny DVD ripper with very few options. The purpose being
	 * that there is as little overhead as possible when wanting to do a quick DVD rip, and
	 * has a good set of default encoding settings.
	 *
	 * Just running "dvd_trip" alone with no arguments or options will fetch the longest track
	 * on the DVD, select the first English audio track, and encode it to H.264 using libx264 with
	 * its default preset and CRF, and AAC audio.
	 *
	 * Because it is so tiny, some options it doesn't have that larger DVD ripper applications
	 * would are: encoding multiple audio streams, audio passthrough, subtitle support (VOBSUB and
	 * closed captioning), auto-cropping black bars, and custom audio and video codec options.
	 *
	 * However(!), if you want more features when ripping the DVD, you can use "dvd_copy" instead,
	 * and output the track to stdout. Using ffmpeg can detect the streams and encode, crop,
	 * copy, and so on directly. Example: dvd_copy -o - | ffmpeg -i - dvd_track.mp4
	 *
	 * To keep things simple, dvd_trip only has three presets which fit the most commonly used
	 * codecs and formats when ripping DVDs: "mp4" and "mkv".
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
	bool opt_last_chapter = false;
	bool opt_filename = false;
	bool opt_start = false;
	bool opt_stop = false;
	char start[DVD_INFO_MPV_TIME_POSITION + 1] = {'\0'};
	char stop[DVD_INFO_MPV_TIME_POSITION + 1] = {'\0'};
	char length[DVD_INFO_MPV_TIME_POSITION + 1] = {'\0'};
	bool x264 = false;
	bool x265 = false;
	bool opus = false;
	bool aac = false;
	bool mkv = false;
	bool mp4 = false;
	bool deinterlace = true;
	int8_t crf = -1;
	char str_crf[10];
	uint16_t arg_track_number = 1;
	int long_index = 0;
	int opt = 0;
	size_t s = 0;
	unsigned long int arg_number = 0;
	uint8_t arg_first_chapter = 1;
	uint8_t arg_last_chapter = 99;
	char *token = NULL;
	char dvd_mpv_first_chapter[4] = {'\0'};
	char dvd_mpv_last_chapter[4] = {'\0'};
	char dvd_mpv_audio_channels[2] = {'\0'};
	const char *home_dir = getenv("HOME");
	char dvd_mpv_args[64];
	int retval = 0;

	// Video Title Set
	struct dvd_vts dvd_vts[99];

	struct dvd_trip dvd_trip;

	dvd_trip.track = 1;
	dvd_trip.first_chapter = 1;
	memset(dvd_trip.filename, '\0', PATH_MAX);
	memset(dvd_trip.config_dir, '\0', PATH_MAX);
	memset(dvd_trip.mpv_config_dir, '\0', PATH_MAX);
	memset(dvd_trip.container, '\0', sizeof(dvd_trip.container));
	memset(dvd_trip.vcodec, '\0', sizeof(dvd_trip.vcodec));
	memset(dvd_trip.vcodec_opts, '\0', sizeof(dvd_trip.vcodec_opts));
	memset(dvd_trip.vcodec_log_level, '\0', sizeof(dvd_trip.vcodec_log_level));
	memset(dvd_trip.acodec, '\0', sizeof(dvd_trip.acodec));
	memset(dvd_trip.acodec_opts, '\0', sizeof(dvd_trip.acodec_opts));
	memset(dvd_trip.mpv_audio_channels, '\0', sizeof(dvd_trip.mpv_audio_channels));
	memset(dvd_trip.audio_lang, '\0', sizeof(dvd_trip.audio_lang));
	memset(dvd_trip.audio_stream_id, '\0', sizeof(dvd_trip.audio_stream_id));
	dvd_trip.encode_subtitles = false;
	memset(dvd_trip.subtitles_lang, '\0', sizeof(dvd_trip.subtitles_lang));
	memset(dvd_trip.subtitles_stream_id, '\0', sizeof(dvd_trip.subtitles_stream_id));
	memset(dvd_trip.vf_opts, '\0', sizeof(dvd_trip.vf_opts));
	memset(dvd_trip.of_opts, '\0', sizeof(dvd_trip.of_opts));
	memset(dvd_trip.start, '\0', sizeof(dvd_trip.start));
	memset(dvd_trip.stop, '\0', sizeof(dvd_trip.stop));
	memset(length, '\0', sizeof(length));
	memset(start, '\0', sizeof(start));
	memset(stop, '\0', sizeof(stop));
	memset(str_crf, '\0', sizeof(str_crf));
	snprintf(dvd_trip.config_dir, PATH_MAX - 1, "/.config/dvd_trip");
	memset(dvd_mpv_first_chapter, '\0', sizeof(dvd_mpv_first_chapter));
	memset(dvd_mpv_last_chapter, '\0', sizeof(dvd_mpv_last_chapter));
	memset(dvd_mpv_args, '\0', sizeof(dvd_mpv_args));
	dvd_trip.video_bitrate = 2048;
	dvd_trip.audio_bitrate = 256;

	if(home_dir != NULL)
		snprintf(dvd_trip.mpv_config_dir, PATH_MAX - 1, "%s%s", home_dir, dvd_trip.config_dir);

	struct option long_options[] = {

		{ "audio", required_argument, 0, 'a' },
		{ "alang", required_argument, 0, 'L' },
		{ "aid", required_argument, 0, 'B' },

		{ "slang", required_argument, 0, 'l' },
		{ "sid", required_argument, 0, 'S' },

		{ "track", required_argument, 0, 't' },
		{ "chapters", required_argument, 0, 'c' },
		{ "start", required_argument, 0, 's' },
		{ "stop", required_argument, 0, 'p' },

		{ "output", required_argument, 0, 'o' },

		{ "no-deinterlace", no_argument, 0, 'D' },

		{ "vcodec", required_argument, 0, 'v'},
		{ "acodec", required_argument, 0, 'a'},
		{ "crf", required_argument, 0, 'q' },

		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },

		{ "verbose", no_argument, 0, 'x' },
		{ "debug", no_argument, 0, 'z' },
		{ 0, 0, 0, 0 }

	};

	while((opt = getopt_long(argc, argv, "a:B:c:DhL:l:o:p:q:s:S:t:T:Vv:xz", long_options, &long_index )) != -1) {

		switch(opt) {

			case 'a':
				if(strncmp(optarg, "aac", 3) == 0) {
					aac = true;
				} else if(strncmp(optarg, "opus", 4) == 0) {
					opus = true;
				}
				break;

			case 'B':
				strncpy(dvd_trip.audio_stream_id, optarg, 3);
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
					else {
						opt_last_chapter = true;
						arg_last_chapter = (uint8_t)arg_number;
					}
				}

				if(arg_last_chapter < arg_first_chapter) {
					fprintf(stderr, "[dvd_trip] Last chapter must be a greater number than first chapter\n");
					return 1;
				}

				if(arg_first_chapter > arg_last_chapter) {
					fprintf(stderr, "[dvd_trip] First chapter must be a lower number than first chapter\n");
					return 1;
				}

				break;

			case 'D':
				deinterlace = false;
				break;

			case 'L':
				strncpy(dvd_trip.audio_lang, optarg, 2);
				break;

			case 'l':
				strncpy(dvd_trip.subtitles_lang, optarg, 2);
				break;

			case 'o':
				opt_filename = true;
				strncpy(dvd_trip.filename, optarg, PATH_MAX - 1);

				if(strcmp(strrchr(optarg, '\0') - 4, ".mkv") == 0) {
					mkv = true;
				} else if(strcmp(strrchr(optarg, '\0') - 4, ".mp4") == 0) {
					mp4 = true;
				}
				break;

			case 'p':
				opt_stop = true;
				strncpy(length, optarg, DVD_INFO_MPV_TIME_POSITION);

				for(s = 0; s < strlen(length); s++) {
					if(!isdigit(length[s]) && !(length[s] == '-' || length[s] == '+' || length[s] == ':' || length[s] == '.' || length[s] == '%')) {
						fprintf(stderr, "[dvd_trip] Invalid length format, use [+|-][[hh:]mm:]ss[.ms] or <percent>%%\n");
						return 1;
					}
				}
				strncpy(stop, length, DVD_INFO_MPV_TIME_POSITION);
				break;

			case 'q':
				arg_number = strtoul(optarg, NULL, 10);
				if(arg_number > 63)
					arg_number = 63;
				crf = (int8_t)arg_number;
				break;

			case 's':
				opt_start = true;
				strncpy(length, optarg, DVD_INFO_MPV_TIME_POSITION);

				for(s = 0; s < strlen(length); s++) {
					if(!isdigit(length[s]) && !(length[s] == '-' || length[s] == '+' || length[s] == ':' || length[s] == '.' || length[s] == '%')) {
						fprintf(stderr, "[dvd_trip] Invalid length format, use [+|-][[hh:]mm:]ss[.ms] or <percent>%%\n");
						return 1;
					}
				}
				strncpy(start, length, DVD_INFO_MPV_TIME_POSITION);
				break;


			case 'S':
				strncpy(dvd_trip.subtitles_stream_id, optarg, 3);
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
				printf("dvd_trip %s\n", PACKAGE_VERSION);
				return 0;

			case 'x':
				verbose = true;
				break;

			case 'v':
				if(strncmp(optarg, "x264", 4) == 0) {
					x264 = true;
				} else if(strncmp(optarg, "x265", 4) == 0) {
					x265 = true;
				}
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
				printf("  -o, --output <filename>       Save to filename (default: dvd_track_##.mp4)\n");
				printf("\n");
				printf("Track selection:\n");
				printf("  -t, --track <#>          	Encode selected track (default: longest)\n");
				printf("  -c, --chapter <#>[-[#]]       Encode chapter number or range (default: all)\n");
				printf("\n");
				printf("Time duration:\n");
				printf("  -s, --start <format>          Start at time length in format in [+|-][[hh:]mm:]ss[.ms] or <percent>%%\n");
				printf("  -p, --stop <format>           Stop at time length in format in [+|-][[hh:]mm:]ss[.ms] or <percent>%%\n");
				printf("\n");
				printf("Language selection:\n");
				printf("  --alang <language>            Select audio language, two character code (default: first audio track)\n");
				printf("  --aid <#>                     Select audio track ID\n");
				printf("  --slang <language>            Select subtitles language, two character code (default: none)\n");
				printf("  --sid <#>                     Select subtitles track ID\n");
				printf("\n");
				printf("Encoding options:\n");
				printf("\n");
				printf("  -v, --vcodec <vcodec>         Video codec <x264|x265>, default: x264\n");
				printf("  -a, --acodec <acodec>         Audio codec (aac|opus), default: aac\n");
				printf("  -q, --crf <#>			Video encoder CRF (x264 and x265)\n");
				printf("  -D, --no-deinterlace          Do not deinterlace video\n");
				printf("\n");
				printf("Defaults:\n");
				printf("\n");
				printf("By default, dvd_trip will encode source to H.264 video with Opus audio in an\n");
				printf("MKV container. If an output filename is given with a different extension,\n");
				printf("it will use the default settings. for those instead. In each case, the default\n");
				printf("presets are used as selected by the codecs as well. Note that mpv must already\n");
				printf("be built with support for these codecs, or dvd_trip will quit.\n\n");
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

	memset(device_filename, '\0', PATH_MAX);
	if (argv[optind])
		strncpy(device_filename, argv[optind], PATH_MAX - 1);
	else
		strncpy(device_filename, DEFAULT_DVD_DEVICE, PATH_MAX - 1);

	/** Begin dvd_trip :) */

	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "[dvd_trip] cannot access %s\n", device_filename);
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
	dvd_logger_cb dvdread_logger_cb = { dvd_info_logger_cb };

	// Open the DVD with libdvdread
	dvdread_dvd = DVDOpen2(NULL, &dvdread_logger_cb, device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "Opening DVD with dvdread %s failed\n", device_filename);
		return 1;
	}

	// Open the DVD with libvdnav
	dvdnav_t *dvdnav_dvd = NULL;
	dvdnav_status_t dvdnav_dvd_status;
	dvdnav_dvd_status = dvdnav_open(&dvdnav_dvd, device_filename);
	if(dvdnav_dvd_status != DVDNAV_STATUS_OK) {
		fprintf(stderr, "Opening DVD with dvdnav %s failed\n", device_filename);
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

	if(vmg_ifo == NULL || vmg_ifo->vmgi_mat == NULL) {
		fprintf(stderr, "[dvd_trip] Opening VMG IFO failed\n");
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

	// Calculating the number of output chapters. Loop through all the audio tracks that
	// are on the title track, and simply choose what's the highest so we definitely get
	// whatever the greatest source is. In some cases, this will mean upmixing to more channels.
	uint8_t audio_track_ix;
	uint8_t audio_max_channels = 0;
	for(audio_track_ix = 0; audio_track_ix < dvd_track.audio_tracks; audio_track_ix++) {

		dvd_trip.audio_channels = dvd_audio_channels(vts_ifo, audio_track_ix);

		if(dvd_trip.audio_channels > audio_max_channels)
			audio_max_channels = dvd_trip.audio_channels;

	}

	// Default last chapter to the number of chapters
	dvd_trip.last_chapter = dvd_track.chapters;

	// Set the proper chapter range
	if(opt_chapter_number && !opt_last_chapter) {

		if(arg_first_chapter > dvd_track.chapters) {
			fprintf(stderr, "[dvd_trip] Starting chapter cannot be larger than %" PRIu8 "\n", dvd_track.chapters);
			return 1;
		}

		// Only set first chapter, last is set above by default
		dvd_trip.first_chapter = arg_first_chapter;

	}

	if(opt_chapter_number && opt_last_chapter) {

		if(arg_last_chapter > dvd_track.chapters) {
			fprintf(stderr, "[dvd_trip] Final chapter cannot be larger than %" PRIu8 "\n", dvd_track.chapters);
			return 1;
		}

		dvd_trip.first_chapter = arg_first_chapter;

		dvd_trip.last_chapter = arg_last_chapter;

	}

	/**
	 * File descriptors and filenames
	 */
	dvd_file_t *dvdread_vts_file = NULL;

	vts = dvd_vts_ifo_number(vmg_ifo, dvd_trip.track);
	vts_ifo = vts_ifos[vts];

	// Open the VTS VOB
	dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_TITLE_VOBS);

	if(strlen(dvd_info.title))
		fprintf(stderr, "[dvd_trip] Disc title: '%s'\n", dvd_info.title);
	if(strlen(dvdread_id))
		fprintf(stderr, "[dvd_trip] Disc ID: '%s'\n", dvdread_id);
	fprintf(stderr, "[dvd_trip] Track: %*" PRIu16 ", Length: %s, Chapters: %*" PRIu8 ", Cells: %*" PRIu8 ", Audio streams: %*" PRIu8 ", Subpictures: %*" PRIu8 "\n", 2, dvd_track.track, dvd_track.length, 2, dvd_track.chapters, 2, dvd_track.cells, 2, dvd_track.audio_tracks, 2, dvd_track.subtitles);

	// Check for track issues
	if(dvd_vts[vts].valid == false) {
		dvd_track.valid = false;
	}

	if(dvd_track.msecs == 0) {
		fprintf(stderr, "[dvd_trip] invalid track - track has zero length\n");
		dvd_track.valid = false;
	}

	if(dvd_track.chapters == 0) {
		fprintf(stderr, "[dvd_trip] invalid track - zero chapters\n");
		dvd_track.valid = false;
	}

	if(dvd_track.cells == 0) {
		fprintf(stderr, "[dvd_trip] invalid track - zero cells\n");
		dvd_track.valid = false;
	}

	if(!dvd_track.valid) {
		fprintf(stderr, "[dvd_trip] track %" PRIu16 "has been flagged as invalid, and encoding it could cause strange problems\b", dvd_trip.track);
	}

	// MPV instance
	mpv_handle *dvd_mpv = NULL;
	dvd_mpv = mpv_create();
	if(dvd_mpv == NULL) {
		fprintf(stderr, "[dvd_trip] could not create an mpv instance\n");
		return 1;
	}
	mpv_set_option_string(dvd_mpv, "config-dir", dvd_trip.mpv_config_dir);
	mpv_set_option_string(dvd_mpv, "config", "yes");
	mpv_set_option_string(dvd_mpv, "terminal", "yes");
	if(debug) {
		mpv_request_log_messages(dvd_mpv, "debug");
		strcpy(dvd_trip.vcodec_log_level, "full");
	} else if (verbose) {
		mpv_request_log_messages(dvd_mpv, "v");
		strcpy(dvd_trip.vcodec_log_level, "info");
	} else {
		mpv_request_log_messages(dvd_mpv, "info");
		strcpy(dvd_trip.vcodec_log_level, "info");
	}

	/** DVD **/
	snprintf(dvd_mpv_args, sizeof(dvd_mpv_args), "dvd://%" PRIu16, dvd_trip.track - 1);
	const char *dvd_mpv_commands[] = { "loadfile", dvd_mpv_args, NULL };
	mpv_set_option_string(dvd_mpv, "dvd-device", device_filename);

	/* Chapters */

	// Starting chapter or time point
	// Don't need to change the start point string (--start), just add # to chapter number string
	if(opt_chapter_number && opt_start) {
		fprintf(stderr, "[dvd_trip] starting position must be either chapter number or time length format\n");
		return 1;
	}

	// Set first chapter if given
	if(!opt_start && !opt_chapter_number) {
		dvd_trip.first_chapter = 1;
	} else if(opt_chapter_number && dvd_trip.first_chapter > 1) {
		snprintf(dvd_trip.start, sizeof(dvd_trip.start), "#%" PRIu8, dvd_trip.first_chapter);
	} else {
		if(!strlen(start))
			strncpy(start, "0", 2);
		strncpy(dvd_trip.start, start, sizeof(dvd_trip.start));
		fprintf(stderr, "[dvd_trip] starting position: #%s\n", dvd_trip.start);
	}
	mpv_set_option_string(dvd_mpv, "start", dvd_trip.start);

	// Don't allow setting last chapter and stop position
	if(opt_last_chapter && opt_stop) {
		fprintf(stderr, "[dvd_trip] stopping position must be either chapter number or time length format\n");
		return 1;
	}

	// To play a chapter range in MPV, the end chapter needs to be incremented to the final one plus one
	// *unless* selecting just the final chapter. dvd_trip -c 1-1 would be mpv --start=#1 --end=#2
	if(dvd_trip.last_chapter != dvd_track.chapters)
		dvd_trip.mpv_last_chapter = dvd_trip.last_chapter + 1;

	// Stop at either length given, or last chapter passed
	if(!opt_stop) {

		if(dvd_trip.last_chapter == dvd_track.chapters)
			dvd_trip.mpv_last_chapter = dvd_trip.last_chapter;

		snprintf(dvd_trip.stop, sizeof(dvd_trip.stop), "#%" PRIu8, dvd_trip.mpv_last_chapter);
		mpv_set_option_string(dvd_mpv, "end", dvd_trip.stop);
		fprintf(stderr, "[dvd_trip] stopping chapter: %" PRIu8 "\n", dvd_trip.last_chapter);

	} else {

		strncpy(dvd_trip.stop, stop, sizeof(dvd_trip.stop));
		mpv_set_option_string(dvd_mpv, "length", dvd_trip.stop);
		fprintf(stderr, "[dvd_trip] stopping position: %s\n", dvd_trip.stop);

	}

	// If you pass a stop parameter that goes past *the chapter stop position*, then it's going to do
	// who knows what.
	if(verbose && opt_stop && dvd_trip.last_chapter == dvd_track.chapters) {
		fprintf(stderr, "[dvd_trip] WARNING: if --stop position given is past the last chapter, libmpv could do something unexpected\n");
	}

	// Default container MP4
	if(!mp4 && !mkv)
		mp4 = true;

	// Default video codec for MP4 and MKV
	if(!x264 && !x265)
		x264 = true;

	/** Filename **/
	if(!opt_filename) {
		if(mp4)
			strcpy(dvd_trip.container, "mp4");
		else if(mkv)
			strcpy(dvd_trip.container, "mkv");
		sprintf(dvd_trip.filename, "dvd_track_%02" PRIu16 ".%s", dvd_trip.track, dvd_trip.container);
	}

	mpv_set_option_string(dvd_mpv, "o", dvd_trip.filename);

	fprintf(stderr, "[dvd_trip] saving to filename \'%s\'\n", dvd_trip.filename);

	/** Video **/

	// Fix input CRF if needed
	if((x264 || x265) && crf > 51)
		crf = 51;
	if(x264 && crf == -1)
		crf = 20;
	if(x265 && crf == -1)
		crf = 22;
	if(crf > -1 && (x264 || x265)) {
		snprintf(dvd_trip.vcodec_opts, 9, "crf=%i", crf);
	}

	/**
	 * For some reason, when passing a chapter range that does *not* start with the
	 * first one, the output file puts the audio stream first (and it is driving me
	 * crazy). mpv-0.37.0
	 * See: ./dvd_trip 1.000.PRIME_SAMPLE.iso -c 2-2
	 */

	// Video codecs and encoding options
	if (x264)
		strcpy(dvd_trip.vcodec, "libx264");
	else if(x265)
		strcpy(dvd_trip.vcodec, "libx265");

	mpv_set_option_string(dvd_mpv, "ovc", dvd_trip.vcodec);

	// Containers
	if(mp4) {
		strcpy(dvd_trip.of_opts, "movflags=+faststart");
		mpv_set_option_string(dvd_mpv, "ofopts", dvd_trip.of_opts);
	}

	if(strlen(dvd_trip.vcodec_opts))
		mpv_set_option_string(dvd_mpv, "ovcopts", dvd_trip.vcodec_opts);

	// Deinterlacing
	strcat(dvd_trip.vf_opts, "bwdif");
	mpv_set_option_string(dvd_mpv, "vf", dvd_trip.vf_opts);

	/** Audio **/

	// Audio codec
	if(opus)
		strcpy(dvd_trip.acodec, "libopus");
	else
		strcpy(dvd_trip.acodec, "aac");

	// There are some DVD tracks with no audio channels, and mpv will die if number is set
	if(audio_max_channels) {

		if(strlen(dvd_trip.audio_lang))
			mpv_set_option_string(dvd_mpv, "alang", dvd_trip.audio_lang);
		else if(strlen(dvd_trip.audio_stream_id))
			mpv_set_option_string(dvd_mpv, "aid", dvd_trip.audio_stream_id);

		mpv_set_option_string(dvd_mpv, "oac", dvd_trip.acodec);

		// Use same channel layout as source; AAC and Opus both support 5.1
		snprintf(dvd_trip.mpv_audio_channels, sizeof(dvd_trip.mpv_audio_channels), "%" PRIu8, audio_max_channels);
		mpv_set_option_string(dvd_mpv, "audio-channels", dvd_trip.mpv_audio_channels);

		// Use a high bitrate by default to guarantee good sound quality
		snprintf(dvd_trip.acodec_opts, sizeof(dvd_trip.acodec_opts), "b=%" PRIu16 "k", dvd_trip.audio_bitrate);
		mpv_set_option_string(dvd_mpv, "oacopts", dvd_trip.acodec_opts);

	}

	/** Subtitles **/
	if(strlen(dvd_trip.subtitles_lang)) {
		fprintf(stderr, "[dvd_trip] burn-in subtitles lang '%s'\n", dvd_trip.subtitles_lang);
		mpv_set_option_string(dvd_mpv, "slang", dvd_trip.subtitles_lang);
	} else if(strlen(dvd_trip.subtitles_stream_id)) {
		fprintf(stderr, "[dvd_trip] burn-in subtitles stream '%s'\n", dvd_trip.subtitles_stream_id);
		mpv_set_option_string(dvd_mpv, "sid", dvd_trip.subtitles_stream_id);
	}

	// ** All encoding and user config options must be set before initialize **
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

	// Print what settings are used at this point
	/*
	fprintf(stderr, "[libmpv] ovc = %s\n", mpv_get_property_string(dvd_mpv, "ovc"));
	fprintf(stderr, "[libmpv] ovcopts = %s\n", mpv_get_property_string(dvd_mpv, "ovcopts"));
	if(strlen(dvd_trip.vf_opts))
		fprintf(stderr, "[libmpv] vf = %s\n", mpv_get_property_string(dvd_mpv, "vf"));
	if(strlen(dvd_trip.audio_lang))
		fprintf(stderr, "[libmpv] alang = %s\n", mpv_get_property_string(dvd_mpv, "alang"));
	else if(strlen(dvd_trip.audio_stream_id))
		fprintf(stderr, "[libmpv] aid = %s\n", mpv_get_property_string(dvd_mpv, "aid"));
	fprintf(stderr, "[libmpv] acodec = %s\n", mpv_get_property_string(dvd_mpv, "oac"));
	printf("[dvd_trip] mpv dvd://%" PRIu16 " # mpv zero-indexes tracks\n", dvd_trip.track - 1);
	fprintf(stderr, "[libmpv] slang = %s\n", mpv_get_property_string(dvd_mpv, "slang"));
	fprintf(stderr, "[libmpv] sid = %s\n", mpv_get_property_string(dvd_mpv, "sid"));
	*/

	mpv_set_option_string(dvd_mpv, "sid", "none");

	mpv_event *dvd_mpv_event = NULL;
	struct mpv_event_log_message *dvd_mpv_log_message = NULL;
	struct mpv_event_end_file *dvd_mpv_eof = NULL;

	retval = 0;

	// Display output
	if(x264 || x265)
		fprintf(stderr, "[dvd_trip] using video codec '%s' and CRF '%i'\n", dvd_trip.vcodec, crf);
	if(verbose) {
		fprintf(stderr, "[dvd_trip] mpv 'vcodecopts=%s'\n", dvd_trip.vcodec_opts);
		fprintf(stderr, "[dvd_trip] mpv 'acodecopts=%s'\n", dvd_trip.acodec_opts);
	}
	if(deinterlace)
		printf("[dvd_trip] deinterlacing video using 'bwdif' filter\n");

	fprintf(stderr, "[dvd_trip] using audio codec '%s' and bitrate '%" PRIu16 "k'\n", dvd_trip.acodec, dvd_trip.audio_bitrate);

	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN)
			break;

		if(dvd_mpv_event->event_id == MPV_EVENT_END_FILE) {

			dvd_mpv_eof = dvd_mpv_event->data;

			if(dvd_mpv_eof->reason == MPV_END_FILE_REASON_QUIT) {

				fprintf(stderr, "[dvd_trip] stopping encode\n");
				break;

			}

			if(dvd_mpv_eof->reason == MPV_END_FILE_REASON_ERROR) {

				retval = 1;
				fprintf(stderr, "[dvd_trip] libmpv hit end of file error, video may have problems at end\n");

				if(dvd_mpv_eof->error == MPV_ERROR_NOTHING_TO_PLAY) {
					fprintf(stderr, "[dvd_trip] no audio or video data to play\n");
				}

			}

			break;

		}

		// Logging output
		if((verbose || debug) && dvd_mpv_event->event_id == MPV_EVENT_LOG_MESSAGE) {
			dvd_mpv_log_message = (struct mpv_event_log_message *)dvd_mpv_event->data;
			fprintf(stderr, "[libmpv] %s", dvd_mpv_log_message->text);
		}

	}

	mpv_terminate_destroy(dvd_mpv);

	fprintf(stderr, "[dvd_trip] encode finished\n");

	if(retval == 0) {
		fprintf(stderr, "[dvd_trip] file saved to '%s'\n", dvd_trip.filename);
	} else {
		fprintf(stderr, "[dvd_trip] encoding errors, file may be incomplete: '%s'\n", dvd_trip.filename);
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
