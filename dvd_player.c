#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
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
#include <mpv/client.h>
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
#include "dvd_player.h"

	/**
	 *      _          _            _
	 *   __| |_   ____| |     _ __ | | __ _ _   _  ___ _ __
	 *  / _` \ \ / / _` |    | '_ \| |/ _` | | | |/ _ \ '__|
	 * | (_| |\ V / (_| |    | |_) | | (_| | |_| |  __/ |
	 *  \__,_| \_/ \__,_|____| .__/|_|\__,_|\__, |\___|_|
	 *                 |_____|_|            |___/
	 *
	 * ** a tiny DVD player using libmpv as backend **
	 */

int main(int argc, char **argv) {

	int retval = 0;
	bool verbose = false;
	bool debug = false;
	char device_filename[PATH_MAX];
	bool invalid_opts = false;
	bool opt_track_number = false;
	bool opt_chapter_number = false;
	bool opt_last_chapter = false;
	unsigned long int arg_number = 0;
	uint16_t arg_track_number = 0;
	uint8_t arg_first_chapter = 1;
	uint8_t arg_last_chapter = 99;
	struct dvd_player dvd_player;
	struct dvd_playback dvd_playback;
	char dvd_mpv_args[64] = {'\0'};
	const char *home_dir = getenv("HOME");


	// Video Title Set
	struct dvd_vts dvd_vts[DVD_MAX_VTS_IFOS];

	// DVD player default options
	memset(dvd_player.config_dir, '\0', PATH_MAX);
	snprintf(dvd_player.config_dir, 20, "/.config/dvd_player");
	memset(dvd_player.mpv_config_dir, '\0', PATH_MAX);
	if(home_dir != NULL)
		snprintf(dvd_player.mpv_config_dir, PATH_MAX - 1, "%s%s", home_dir, dvd_player.config_dir);

	// DVD playback default options
	dvd_playback.track = 1;
	dvd_playback.first_chapter = 1;
	dvd_playback.last_chapter = 99;
	dvd_playback.fullscreen = false;
	dvd_playback.deinterlace = true;
	dvd_playback.subtitles = false;
	memset(dvd_playback.audio_lang, '\0', sizeof(dvd_playback.audio_lang));
	memset(dvd_playback.audio_stream_id, '\0', sizeof(dvd_playback.audio_stream_id));
	memset(dvd_playback.subtitles_lang, '\0', sizeof(dvd_playback.subtitles_lang));
	memset(dvd_playback.subtitles_stream_id, '\0', sizeof(dvd_playback.subtitles_stream_id));
	memset(dvd_playback.mpv_first_chapter, '\0', sizeof(dvd_playback.mpv_first_chapter));
	memset(dvd_playback.mpv_last_chapter, '\0', sizeof(dvd_playback.mpv_last_chapter));

	int long_index = 0;
	int opt = 0;
	char *token = NULL;

	struct option long_options[] = {

		{ "alang", required_argument, 0, 'a' },
		{ "aid", required_argument, 0, 'A' },
		{ "chapters", required_argument, 0, 'c' },
		{ "no-deinterlace", no_argument, 0, 'D' },
		{ "fullscreen", no_argument, 0, 'f' },
		{ "help", no_argument, 0, 'h' },
		{ "slang", required_argument, 0, 's' },
		{ "sid", required_argument, 0, 'S' },
		{ "track", required_argument, 0, 't' },
		{ "verbose", no_argument, 0, 'v' },
		{ "version", no_argument, 0, 'V' },
		{ "debug", no_argument, 0, 'z' },
		{ 0, 0, 0, 0 }

	};

	while((opt = getopt_long(argc, argv, "Aa:c:dfhS:s:t:vVz", long_options, &long_index )) != -1) {

		switch(opt) {

			case 'A':
				strncpy(dvd_playback.audio_stream_id, optarg, 3);
				break;

			case 'a':
				strncpy(dvd_playback.audio_lang, optarg, 2);
				break;

			case 'c':
				opt_chapter_number = true;
				token = strtok(optarg, "-");
				if(strlen(token) > 2) {
					fprintf(stderr, "[dvd_player] Chapter range must be between 1 and 99\n");
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
						fprintf(stderr, "[dvd_player] Chapter range must be between 1 and 99\n");
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
					fprintf(stderr, "[dvd_player] Last chapter must be a greater number than first chapter\n");
					return 1;
				}

				if(arg_first_chapter > arg_last_chapter) {
					fprintf(stderr, "[dvd_player] First chapter must be a lower number than first chapter\n");
					return 1;
				}
				break;

			case 'D':
				dvd_playback.deinterlace = false;
				break;

			case 'f':
				dvd_playback.fullscreen = true;
				break;

			case 's':
				strncpy(dvd_playback.subtitles_lang, optarg, 2);
				dvd_playback.subtitles = true;
				break;

			case 'S':
				arg_number = strtoul(optarg, NULL, 10);
				if(arg_number > 0 && arg_number < 100) {
					sprintf(dvd_playback.subtitles_stream_id, "%" PRIu8, (uint8_t)arg_number);
					dvd_playback.subtitles = true;
				} else {
					fprintf(stderr, "[dvd_player] subtitle stream ID must be between 1 and 99\n");
					return 1;
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

			case 'v':
				verbose = true;
				break;

			case 'V':
				printf("dvd_player %s\n", PACKAGE_VERSION);
				return 0;
				break;

			case 'z':
				verbose = true;
				debug = true;
				break;

			case '?':
				invalid_opts = true;
			case 'h':
				printf("dvd_player - a tiny DVD player\n");
				printf("\n");
				printf("Usage: dvd_player [path] [options]\n");
				printf("\n");
				printf("Options:\n");
				printf("  -f, --fullscreen              Display in fullscreen mode\n");
				printf("  -t, --track <#>               Playback track number (default: longest valid)\n");
				printf("  -c, --chapter <#>[-#]         Playback chapter number or range (default: all)\n");
				printf("  -a, --alang <language>        Select audio language, two character code (default: first audio track)\n");
				printf("  -A, --aid <#>                 Select audio track ID\n");
				printf("  -s, --slang <language>        Select subtitles language, two character code (default: no subtitles)\n");
				printf("  -S, --sid <#>                 Select subtitles track ID\n");
				printf("  -D, --no-deinterlace          Do not deinterlace video\n");
				printf("  -v, --verbose                 Show verbose output\n");
				printf("  -z, --debug                   Show debugging output\n");
				printf("  -h, --help			Show this help text and exit\n");
				printf("\n");
				printf("DVD path can be a device name, a single file, or directory (default: %s)\n", DEFAULT_DVD_DEVICE);
				printf("\n");
				printf("dvd_player reads a configuration file from ~/.config/dvd_player/mpv.conf\n");
				printf("See mpv man page for syntax and dvd_player man page for examples.\n");
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

	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "[dvd_player] cannot access %s\n", device_filename);
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
	dvdread_dvd = DVDOpen2(NULL, &dvdread_logger_cb, device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "[dvd_player] Opening DVD %s failed\n", device_filename);
		return 1;
	}

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || vmg_ifo->vmgi_mat == NULL) {
		fprintf(stderr, "[dvd_player] Opening VMG IFO failed\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// DVD
	struct dvd_info dvd_info;
	memset(dvd_info.dvdread_id, '\0', sizeof(dvd_info.dvdread_id));
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	dvd_info.tracks = dvd_tracks(vmg_ifo);
	dvd_info.longest_track = 1;

	dvd_title(dvd_info.title, device_filename);

	uint16_t num_ifos;
	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	if(num_ifos < 1) {
		fprintf(stderr, "[dvd_player] DVD has no title IFOs\n");
		return 1;
	}


	// Track
	struct dvd_track dvd_track;
	memset(&dvd_track, 0, sizeof(dvd_track));

	struct dvd_track dvd_tracks[100];
	memset(&dvd_tracks, 0, sizeof(dvd_track) * dvd_info.tracks);

	// Open first IFO
	uint16_t vts = 1;
	ifo_handle_t *vts_ifo = NULL;

	vts_ifo = ifoOpen(dvdread_dvd, vts);
	if(vts_ifo == NULL) {
		fprintf(stderr, "[dvd_player] Could not open primary Video Title Set info\n");
		return 1;
	}
	ifoClose(vts_ifo);
	vts_ifo = NULL;

	// Create an array of all the IFOs
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	ifo_handle_t *vts_ifos[DVD_MAX_VTS_IFOS];
	uint8_t vts_ifo_ix;
	for(vts_ifo_ix = 0; vts_ifo_ix < DVD_MAX_VTS_IFOS; vts_ifo_ix++)
		vts_ifos[vts_ifo_ix] = NULL;

	for(vts = 1; vts < dvd_info.video_title_sets + 1; vts++) {

		dvd_vts[vts].vts = vts;
		dvd_vts[vts].valid = false;

		vts_ifos[vts] = ifoOpen(dvdread_dvd, vts);

		if(vts_ifos[vts] == NULL) {
			dvd_vts[vts].valid = false;
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
		fprintf(stderr, "[dvd_player] Choose a track number between 1 and %" PRIu16 "\n", dvd_info.tracks);
		return 1;
	} else if(opt_track_number) {
		dvd_playback.track = arg_track_number;
	}

	uint16_t ix = 0;
	uint16_t track_number = 1;

	uint32_t msecs = 0;
	uint32_t longest_msecs = 0;

	// If no track number is given, choose the longest one that is valid and also
	// has active audio tracks.
	if(!opt_track_number) {

		for(ix = 0, track_number = 1; ix < dvd_info.tracks; ix++, track_number++) {

			vts = dvd_vts_ifo_number(vmg_ifo, track_number);
			vts_ifo = vts_ifos[vts];

			if(dvd_vts[vts].valid == false)
				continue;

			if(dvd_audio_active_tracks(vmg_ifo, vts_ifo, track_number) == 0)
				continue;

			if(dvd_track_chapters(vmg_ifo, vts_ifo, track_number) == 0)
				continue;

			if(dvd_track_cells(vmg_ifo, vts_ifo, track_number) == 0)
				continue;

			msecs = dvd_track_msecs(vmg_ifo, vts_ifo, track_number);

			if(msecs > longest_msecs) {
				longest_msecs = msecs;
				dvd_playback.track = track_number;
			}

		}

	}

	if(vts_ifo)
		ifoClose(vts_ifo);

	dvd_track = dvd_tracks[dvd_playback.track];
	dvd_track.track = dvd_playback.track;
	dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, dvd_track.track);
	vts_ifo = ifoOpen(dvdread_dvd, dvd_track.vts);
	dvd_track_length(dvd_track.length, vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.chapters = dvd_track_chapters(vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.filesize_mbs = dvd_track_filesize_mbs(vmg_ifo, vts_ifo, dvd_track.track);

	if(vts_ifo)
		ifoClose(vts_ifo);

	if(vmg_ifo)
		ifoClose(vmg_ifo);

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	// Set the proper chapter range
	if(opt_chapter_number) {
		if(arg_first_chapter > dvd_track.chapters) {
			dvd_playback.first_chapter = dvd_track.chapters;
			fprintf(stderr, "[dvd_player] Starting chapter cannot be larger than %" PRIu8 "\n", dvd_track.chapters);
			return 1;
		} else
			dvd_playback.first_chapter = arg_first_chapter;

		if(arg_last_chapter > dvd_track.chapters) {
			dvd_playback.last_chapter = dvd_track.chapters;
			fprintf(stderr, "[dvd_player] Final chapter cannot be larger than %" PRIu8 "\n", dvd_track.chapters);
			return 1;
		} else
			dvd_playback.last_chapter = arg_last_chapter;
	} else {
		dvd_playback.first_chapter = 1;
		dvd_playback.last_chapter = dvd_track.chapters;
	}

	// DVD playback using libmpv
	mpv_handle *dvd_mpv = NULL;
	dvd_mpv = mpv_create();
	if(dvd_mpv == NULL) {
		fprintf(stderr, "[dvd_player] could not create new MPV instance\n");
		return 1;
	}

	// Terminal output
	mpv_set_option_string(dvd_mpv, "terminal", "yes");

	// [ffmpeg/audio] 'frame sync errors' which are normal when seeking on DVDs
	if(debug) {
		mpv_request_log_messages(dvd_mpv, "debug");
	} else if(verbose) {
		mpv_request_log_messages(dvd_mpv, "v");
	} else {
		mpv_request_log_messages(dvd_mpv, "none");
	}

	/** DVD Track Selection */
	/*
	 * One thing I try to do with dvd_info (and friends) is closely check if a track is valid
	 * or not. When no track is passed, I iterate through all of them to see which one is the
	 * longest *and* valid. This is just an additional check. If no argument is passed, I could
	 * hand it off to libdvdnav to follow the program chain.
	 *
	 * Different players do different things, not just based on their default playback library,
	 * but also on what they support. VLC, for example, will use dvdnav to display the navigation
	 * menus, and let the user select from there. MPlayer will also display the menus. MPV
	 * doesn't have that option. In most cases, other DVD players will just choose the longest
	 * track and play that one back, valid or not. This chooses the longest track that is also valid.
	 *
	 * There's always the chance of running into DVDs that have tracks marked as valid, but
	 * are still garbage. Unfortunately, it's still going to choose those. Other players
	 * will do the same thing as well, if it's the longest. The only way to choose the correct
	 * track each time is to navigate through dvd_info output, find the correct track you want,
	 * and then choose that one for playback.
	 *
	 * MPV *zero* indexes its tracks, while dvd_info starts track indexes at 1.
	 * For example, dvd_info's track 12 will be mpv's track 11. Because of that, you'll see output
	 * from libmpv that it's playing that track, which can be kind of confusing, obviously.
	 * By comparison, HandBrake, VLC, mplayer, and lsdvd also index tracks starting at 1.
	 *
	 * dvd_player takes the same track index as dvd_info, and gives libmpv its relevant track ID.
	 *
	 */
	memset(dvd_mpv_args, '\0', sizeof(dvd_mpv_args));
	snprintf(dvd_mpv_args, sizeof(dvd_mpv_args), "dvd://%" PRIu16, dvd_playback.track - 1);
	const char *dvd_mpv_commands[] = { "loadfile", dvd_mpv_args, NULL };

	// Load user's mpv configuration in ~/.config/dvd_player/mpv.conf (and friends)
	if(strlen(dvd_player.mpv_config_dir)) {
		mpv_set_option_string(dvd_mpv, "config-dir", dvd_player.mpv_config_dir);
		mpv_set_option_string(dvd_mpv, "config", "yes");
	}

	/** DVD Chapter Range */
	/*
	 * When using dvd_player, the '--chapter' option can playback all chapters by default,
	 * select one specific chapter, or a range of chapters.
	 *
	 * MPV has a very smart and nice way to parse lots of start / stop options with the
	 * '--start' option they have. When playing one chapter, it expects the end one to
	 * be the starting one incremented by 1. Syntax for playing only chapter 1 would be:
	 * 'mpv dvd:// --start=#1 --end=#2'
	 *
	 * By default, MPV will just play all chapters of the selected track.
	 */
	if(opt_chapter_number) {

		if(dvd_playback.last_chapter == dvd_playback.first_chapter && dvd_playback.last_chapter < dvd_track.chapters && opt_last_chapter)
			dvd_playback.last_chapter += 1;

		// Do *not* pad last chapter (see dvd_rip.c for same logic)
		if(!opt_last_chapter)
			dvd_playback.last_chapter = dvd_track.chapters;

		snprintf(dvd_playback.mpv_first_chapter, sizeof(dvd_playback.mpv_first_chapter), "#%" PRIu8, dvd_playback.first_chapter);
		mpv_set_option_string(dvd_mpv, "start", dvd_playback.mpv_first_chapter);

	} else {

		// Work around libmpv oddities -- based on whether you seek by fast forwarding or by chapter
		// number, it *might* wrap around. So by default, tell it to quit at the last chapter.
		// Also note that if going to the last chapter, it does *not* get padded by 1
		dvd_playback.last_chapter = dvd_track.chapters;

	}

	// Always set last chapter
	snprintf(dvd_playback.mpv_last_chapter, sizeof(dvd_playback.mpv_last_chapter), "#%" PRIu8, dvd_playback.last_chapter);
	mpv_set_option_string(dvd_mpv, "end", dvd_playback.mpv_last_chapter);

	// Playback options and default configuration
	mpv_set_option_string(dvd_mpv, "dvd-device", device_filename);
	mpv_set_option_string(dvd_mpv, "title", "DVD Player");
	mpv_set_option_string(dvd_mpv, "input-default-bindings", "yes");
	mpv_set_option_string(dvd_mpv, "input-vo-keyboard", "yes");
	if(dvd_playback.fullscreen)
		mpv_set_option_string(dvd_mpv, "fullscreen", NULL);

	/** Video Playback */
	/**
	 * mpv does have the option of passing a video stream id, using '--vid', and also
	 * allowing the argument of 'no' to display no video at all. I *could* add those
	 * options to the program, but in the interest of keeping this tiny, I'm choosing
	 * not to.
	 *
	 * There is the option of adding it as a hidden feature, or document it in the man
	 * page at some point, but as far as right now, I'm not interested. Mostly because
	 * it's a feature I wouldn't use. If you want to *watch* a DVD, then watch it. ;)
	 *
	 * If the user wants video always disabled, it can be set in 'mpv.conf'.
	 */

	// Deinterlace video by default
	if(dvd_playback.deinterlace)
		mpv_set_option_string(dvd_mpv, "vf", "bwdif");

	/** Audio Playback */
	/*
	 * Order of operations for choosing an audio track is: user's mpv.conf, argument
	 * passed to '--alang', and finally the stream id, '--aid'.
	 *
	 * Choosing the language itself is generally a generic call, and this hands over to
	 * mpv which one to choose. I've looked in the past at the option of mapping the same
	 * stream id that dvd_info uses, but I haven't had any luck getting down into the details.
	 * The ffmpeg stream ids are a different index, and don't match dvd_info's. Trying to
	 * parse and figure out the correct order would be a lot of code, for very little gain.
	 * If the user knows which stream id they want, choose it.
	 *
	 * Same as video, I don't have the option right now to do no audio. Set it in 'mpv.conf'
	 */
	if(strlen(dvd_playback.audio_lang))
		mpv_set_option_string(dvd_mpv, "alang", dvd_playback.audio_lang);
	else if(strlen(dvd_playback.audio_stream_id))
		mpv_set_option_string(dvd_mpv, "aid", dvd_playback.audio_stream_id);

	/** Displaying Subtitles */
	/**
	 * When doing normal playback using "mpv dvd://", the user has the option to cycle
	 * through the available subtitles. With dvd_player, they can only be turned on,
	 * and they will stay on. There's no option to cycle through other ones (hence, why
	 * it's a tiny player).
	 *
	 * The language to playback or stream id is first read from the user's mpv.conf file.
	 * After that, it looks at arguments passed to the player.
	 *
	 * Passing a stream id is usually considered to be more specific, so it will override
	 * whatever subtitles language is chosen.
	 *
	 */
	if(dvd_playback.subtitles && strlen(dvd_playback.subtitles_lang))
		mpv_set_option_string(dvd_mpv, "slang", dvd_playback.subtitles_lang);
	else if(dvd_playback.subtitles && strlen(dvd_playback.subtitles_stream_id))
		mpv_set_option_string(dvd_mpv, "sid", dvd_playback.subtitles_stream_id);

	// setup mpv
	retval = mpv_initialize(dvd_mpv);
	if(retval) {
		fprintf(stderr, "[dvd_player] could not initialize MPV\n");
		return 1;
	}

	// start playback
	retval = mpv_command(dvd_mpv, dvd_mpv_commands);
	if(retval) {
		fprintf(stderr, "[dvd_player] could not send DVD playback commands: %s\n", dvd_mpv_args);
		return 1;
	}

	fprintf(stderr, "[dvd_player] Track: %" PRIu16 ", Length: %s, Chapters: %" PRIu8 ", Filesize: %.0lf MBs\n", dvd_playback.track, dvd_track.length, dvd_track.chapters, dvd_track.filesize_mbs);

	fprintf(stderr, "[dvd_player] starting at chapter #%" PRIu8 "\n", dvd_playback.first_chapter);
	fprintf(stderr, "[dvd_player] stopping at chapter #%" PRIu8 "\n", dvd_playback.last_chapter);

	struct mpv_event_log_message *dvd_mpv_log_message = NULL;
	mpv_event *dvd_mpv_event = NULL;
	struct mpv_event_end_file *dvd_mpv_eof = NULL;
	retval = 0;
	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		if(dvd_mpv_event->event_id == MPV_EVENT_END_FILE) {

			dvd_mpv_eof = dvd_mpv_event->data;

			if(dvd_mpv_eof->reason == MPV_END_FILE_REASON_QUIT) {
				break;
			}

			if(dvd_mpv_eof->reason == MPV_END_FILE_REASON_ERROR) {

				retval = 1;
				fprintf(stderr, "[libmpv] end of file error\n");

				if(dvd_mpv_eof->error == MPV_ERROR_NOTHING_TO_PLAY) {
					fprintf(stderr, "[libmpv] no audio or video data to play\n");
				}

			}

			break;

		}

		// Goodbye :)
		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN)
			break;

		// Logging output
		if((verbose || debug) && dvd_mpv_event->event_id == MPV_EVENT_LOG_MESSAGE) {
			dvd_mpv_log_message = (struct mpv_event_log_message *)dvd_mpv_event->data;
			printf("[libmpv] [%s]: %s", dvd_mpv_log_message->level, dvd_mpv_log_message->text);
		}

	}

	mpv_terminate_destroy(dvd_mpv);

	return retval;

}
