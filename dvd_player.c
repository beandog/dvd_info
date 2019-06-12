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
#define DVD_INFO_VERSION "1.7_beta1"
#endif

int main(int argc, char **argv) {

	int retval = 0;
	bool verbose = false;
	bool debug = false;
	bool invalid_opts = false;
	bool opt_track_number = false;
	bool opt_chapter_number = false;
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
	snprintf(dvd_player.config_dir, 20, "/.config/dvd_player");
	memset(dvd_player.mpv_config_dir, '\0', sizeof(dvd_player.mpv_config_dir));
	if(home_dir != NULL)
		snprintf(dvd_player.mpv_config_dir, PATH_MAX - 1, "%s%s", home_dir, dvd_player.config_dir);

	// DVD playback default options
	dvd_playback.track = 1;
	dvd_playback.first_chapter = 1;
	dvd_playback.last_chapter = 99;
	dvd_playback.fullscreen = false;
	dvd_playback.deinterlace = false;
	dvd_playback.subtitles = false;
	memset(dvd_playback.audio_lang, '\0', sizeof(dvd_playback.audio_lang));
	memset(dvd_playback.audio_stream_id, '\0', sizeof(dvd_playback.audio_stream_id));
	memset(dvd_playback.subtitles_lang, '\0', sizeof(dvd_playback.subtitles_lang));
	memset(dvd_playback.subtitles_stream_id, '\0', sizeof(dvd_playback.subtitles_stream_id));

	int long_index = 0;
	int opt = 0;
	char *token = NULL;

	struct option long_options[] = {

		{ "alang", required_argument, 0, 'a' },
		{ "aid", required_argument, 0, 'A' },
		{ "chapters", required_argument, 0, 'c' },
		{ "deinterlace", no_argument, 0, 'd' },
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

	while((opt = getopt_long(argc, argv, "Aa:c:dfhS:s:t:Vvz", long_options, &long_index )) != -1) {

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
					else
						arg_last_chapter = (uint8_t)arg_number;
				}

				if(arg_last_chapter < arg_first_chapter)
					arg_last_chapter = arg_first_chapter;

				if(arg_first_chapter > arg_last_chapter)
					arg_first_chapter = arg_last_chapter;
				break;

			case 'd':
				dvd_playback.deinterlace = true;
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

			case 'V':
				printf("dvd_player %s\n", DVD_INFO_VERSION);
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
				printf("dvd_player - a tiny DVD player\n");
				printf("\n");
				printf("Usage: dvd_player [path] [options]\n");
				printf("\n");
				printf("Options:\n");
				printf("  -f, --fullscreen              Display in fullscreen mode\n");
				printf("  -d, --deinterlace             Deinterlace video\n");
				printf("  -t, --track <#>               Playback track number (default: longest valid)\n");
				printf("  -c, --chapter <#>[-#]         Playback chapter number or range (default: all)\n");
				printf("  -a, --alang <language>        Select audio language, two character code (default: first audio track)\n");
				printf("  -A, --aid <#>                 Select audio track ID\n");
				printf("  -s, --slang <language>        Select subtitles language, two character code (default: no subtitles)\n");
				printf("  -S, --sid <#>                 Select subtitles track ID\n");
				printf("  -v, --verbose                 Verbose output\n");
				printf("  -h, --help			Show this help text and exit\n");
				printf("      --version                 Show version info and exit\n");
				printf("\n");
				printf("DVD path can be a device name, a single file, or directory (default: %s)\n", DEFAULT_DVD_DEVICE);
				printf("\n");
				printf("dvd_trip reads a configuration file from ~/.config/dvd_trip/mpv.conf\n");
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

	const char *device_filename = DEFAULT_DVD_DEVICE;

	if (argv[optind])
		device_filename = argv[optind];

	if(access(device_filename, F_OK) != 0) {
		fprintf(stderr, "[dvd_player] cannot access %s\n", device_filename);
		return 1;
	}

	// Check to see if device can be opened
	int dvd_fd = 0;
	dvd_fd = dvd_device_open(device_filename);
	if(dvd_fd < 0) {
		fprintf(stderr, "[dvd_player] error opening %s\n", device_filename);
		return 1;
	}
	dvd_device_close(dvd_fd);

#ifdef __linux__

	// Poll drive status if it is hardware
	if(dvd_device_is_hardware(device_filename)) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "[dvd_player] drive status: ");
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
		fprintf(stderr, "[dvd_player] Opening DVD %s failed\n", device_filename);
		return 1;
	}

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
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

	uint16_t num_ifos = 1;
	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;

	if(num_ifos < 1) {
		fprintf(stderr, "[dvd_player] DVD has no title IFOs\n");
		return 1;
	}


	// Track
	struct dvd_track dvd_track;
	memset(&dvd_track, 0, sizeof(dvd_track));

	struct dvd_track dvd_tracks[DVD_MAX_TRACKS];
	memset(&dvd_tracks, 0, sizeof(dvd_track) * dvd_info.tracks);

	// Open first IFO
	uint16_t vts = 1;
	ifo_handle_t *vts_ifo = NULL;

	vts_ifo = ifoOpen(dvdread_dvd, vts);
	if(vts_ifo == NULL) {
		fprintf(stderr, "[dvd_player] Could not open primary VTS IFO\n");
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
	
	// If no track number is given, choose the longest one that is also valid
	if(!opt_track_number) {

		for(ix = 0, track_number = 1; ix < dvd_info.tracks; ix++, track_number++) {

			vts = dvd_vts_ifo_number(vmg_ifo, track_number);
			vts_ifo = vts_ifos[vts];

			if(dvd_vts[vts].valid == false)
				continue;

			msecs = dvd_track_msecs(vmg_ifo, vts_ifo, track_number);
			if(msecs == 0)
				continue;

			if(dvd_track_chapters(vmg_ifo, vts_ifo, track_number) == 0)
				continue;

			if(dvd_track_cells(vmg_ifo, vts_ifo, track_number) == 0)
				continue;

			if(msecs > longest_msecs) {
				longest_msecs = msecs;
				dvd_playback.track = track_number;
			}

		}
	
	}

	dvd_track = dvd_tracks[dvd_playback.track - 1];

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
		} else
			dvd_playback.first_chapter = arg_first_chapter;
		
		if(arg_last_chapter > dvd_track.chapters) {
			dvd_playback.last_chapter = dvd_track.chapters;
		} else
			dvd_playback.last_chapter = arg_last_chapter;
	} else {
		dvd_playback.first_chapter = 1;
		dvd_playback.last_chapter = dvd_track.chapters;
	}
	
	printf("[dvd_player] track: %02" PRIu16 ", length: %s, chapters: %" PRIu8 ", cells: %" PRIu8 ", audio streams: %" PRIu8 ", subpictures: %" PRIu8 ", blocks: %zd, filesize: %zd\n", dvd_playback.track, dvd_track.length, dvd_track.chapters, dvd_track.cells, dvd_track.audio_tracks, dvd_track.subtitles, dvd_track.blocks, dvd_track.filesize);

	// DVD playback using libmpv
	mpv_handle *dvd_mpv = NULL;
	dvd_mpv = mpv_create();

	// Terminal output
	mpv_set_option_string(dvd_mpv, "terminal", "yes");
	mpv_set_option_string(dvd_mpv, "term-osd-bar", "yes");

	if(debug) {
		mpv_request_log_messages(dvd_mpv, "debug");
	} else if(verbose) {
		mpv_request_log_messages(dvd_mpv, "v");
	} else {
		mpv_request_log_messages(dvd_mpv, "none");
		// Skip "[ffmpeg/audio] ac3: frame sync error" which are normal when seeking on DVDs
		mpv_set_option_string(dvd_mpv, "msg-level", "ffmpeg/audio=none");
	}

	/** DVD Playback Engine */
	/*
	 * MPV supports playback through both libdvdread and libdvdnav. libdvdnav is the default
	 * used if you pass 'dvd://' as the source input. To use libdvdread specifically, you
	 * use 'dvdread://' instead, which is what I'm doing below.
	 *
	 * One thing I try to do with dvd_info (and friends) is closely check if a track is valid
	 * or not. When no track is passed, I iterate through all of them to see which one is the
	 * longest *and* valid. This is just an additional check. If no argument is passed, I could
	 * hand it off to libdvdnav to follow the program chain.
	 *
	 * Different player do different things, not just based on their default playback library,
	 * but also on what they support. VLC, for example, will use dvdnav to display the navigation
	 * menus, and let the user select from there. MPlayer will also display the menus. MPV
	 * doesn't have that option. In most cases, other DVD players will just choose the longest
	 * track and play that one back. I do the same thing, setting it specifically instead of
	 * letting the player do its own work. The outcome is the same.
	 *
	 * There's always the chance of running into DVDs that have tracks marked as valid, but
	 * are still garbage. Unfortunately, it's still going to choose those (and other players
	 * will do the same thing as well, if it's the longest). The only way to choose the correct
	 * track each time is to navigate through the tree, find the one that has your video on it,
	 * and then choose that one for playback.
	 */

	/** DVD Track Selection */
	/*
	 * The first thing to note is that MPV *zero* indexes its tracks, while dvd_info *one*
	 * indexes them. So in dvd_info, track 12, is track 11. dvd_info -t 12 vs mpv dvd://10
	 * Because of that, you'll see output from libmpv that it's playing that track, which
	 * can be kind of confusing, obviously.
	 *
	 * HandBrake, VLC, mplayer, and lsdvd also index starting at 1.
	 *
	 */
	memset(dvd_mpv_args, '\0', sizeof(dvd_mpv_args));
	sprintf(dvd_mpv_args, "dvdread://%" PRIu16, dvd_playback.track - 1);
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
	 * be the starting one incremented by 1. Syntax for playing only chapter one would be:
	 * 'mpv dvdread:// --start=#1 --end=#2'
	 *
	 * By default, MPV will just play all chapters of the selected track.
	 */
	if(opt_chapter_number) {
		memset(dvd_playback.mpv_chapters_range, '\0', sizeof(dvd_playback.mpv_chapters_range));
		sprintf(dvd_playback.mpv_chapters_range, "%" PRIu8 "-%" PRIu8, dvd_playback.first_chapter, dvd_playback.last_chapter + 1);
		mpv_set_option_string(dvd_mpv, "chapter", dvd_playback.mpv_chapters_range);
	}

	// Playback options and default configuration
	mpv_set_option_string(dvd_mpv, "dvd-device", device_filename);
	if(strlen(dvd_info.title))
		mpv_set_option_string(dvd_mpv, "title", dvd_info.title);
	else
		mpv_set_option_string(dvd_mpv, "title", "dvd_player");
	mpv_set_option_string(dvd_mpv, "input-default-bindings", "yes");
	mpv_set_option_string(dvd_mpv, "input-vo-keyboard", "yes");
	if(dvd_playback.fullscreen)
		mpv_set_option_string(dvd_mpv, "fullscreen", NULL);

	/** Video Playback */
	/**
	 * mpv does have the option of passing a video stream id, using '--vid', and also
	 * allowing the argument of 'no' to display no video at all. I *could* add those
	 * options to the program, but in the interest of keeping this tiny, I'm choosing
	 * not to. dvd_trip, however, does allow it, in case you might want just audio.
	 *
	 * There is the option of adding it as a hidden feature, or document it in the man
	 * page at some point, but as far as right now, I'm not interested. Mostly because
	 * it's a feature I wouldn't use. If you want to *watch* a DVD, then watch it. ;)
	 *
	 * If the user wants video always disabled, it can be set in 'mpv.conf'.
	 */
	/*
	bool opt_no_video = false;
	if(opt_no_video)
		mpv_set_option_string(dvd_mpv, "video", "no");
	*/

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
	if(strlen(dvd_playback.audio_stream_id))
		mpv_set_option_string(dvd_mpv, "aid", dvd_playback.audio_stream_id);
	else if(strlen(dvd_playback.audio_lang))
		mpv_set_option_string(dvd_mpv, "alang", dvd_playback.audio_lang);
	/*
	bool opt_no_audio = false;
	if(opt_no_audio)
		mpv_set_option_string(dvd_mpv, "audio", "no");
	*/

	/** Displaying Subtitles */
	/**
	 * When doing normal playback using "mpv dvd://", the user has the option to cycle
	 * through the available subtitles. With dvd_player, they can only be turned on,
	 * and they will stay on. There's no option to cycle through other ones (hence, why
	 * it's a tiny player).
	 *
	 * The language to display or stream id is first read from the user's mpv.conf file.
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

	/** Deinterlacing Video */

	/*
	 * MPV has a --deinterlace option, but I haven't seen it actually work.
	 * Instead, I'm enabling the yadif (yet another deinterlacing filter) filter option,
	 * with the same setting as in dvd_trip. It is version dependent, however, as the
	 * API upstream changes.
	 *
	 * dvd_trip also requires the FPS when 'encoding', but that's not needed here since
	 * we're just doing playback.
	 */
	if(dvd_playback.deinterlace) {

		if(mpv_client_api_version() <= MPV_MAKE_VERSION(1, 25)) {
			// Syntax up to 0.27.2
			mpv_set_option_string(dvd_mpv, "vf", "lavfi=yadif");
		} else {
			// Syntax starting in 0.29.1
			mpv_set_option_string(dvd_mpv, "vf", "lavfi-yadif");
		}

	}

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

	struct mpv_event_log_message *dvd_mpv_log_message = NULL;
	mpv_event *dvd_mpv_event = NULL;
	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		// Goodbye :)
		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN || dvd_mpv_event->event_id == MPV_EVENT_END_FILE)
			break;

		// Logging output
		if((verbose || debug) && dvd_mpv_event->event_id == MPV_EVENT_LOG_MESSAGE) {
			dvd_mpv_log_message = (struct mpv_event_log_message *)dvd_mpv_event->data;
			printf("[libmpv] [%s]: %s", dvd_mpv_log_message->level, dvd_mpv_log_message->text);
		}

	}

	mpv_terminate_destroy(dvd_mpv);

	return 0;

}
