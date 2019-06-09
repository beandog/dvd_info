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
#include "dvd_vmg_ifo.h"
#include "dvd_vts.h"
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

#define DVD_INFO_PROGRAM "dvd_player"

int main(int, char **);
void dvd_track_info(struct dvd_track *dvd_track, const uint16_t track_number, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo);
void print_usage(char *binary);
void print_version(char *binary);

struct dvd_player {
	char config_dir[20];
	char mpv_config_dir[PATH_MAX - 1];
};

struct dvd_playback {
	uint16_t track;
	uint8_t first_chapter;
	uint8_t last_chapter;
	bool fullscreen;
	bool deinterlace;
	char audio_lang[3];
	char audio_aid[4];
	bool subtitles;
	char subtitles_lang[3];
	char subtitles_sid[4];
	char mpv_chapters_range[8];
};

int main(int argc, char **argv) {

	/**
	 * Parse options
	 */

	bool verbose = false;
	bool debug = false;
	bool opt_track_number = false;
	bool opt_chapter_number = false;
	bool opt_widescreen = false;
	bool opt_pan_scan = false;
	bool opt_no_video = false;
	bool opt_no_audio = false;
	uint16_t arg_track_number = 0;
	uint8_t arg_first_chapter = 1;
	uint8_t arg_last_chapter = 99;
	struct dvd_player dvd_player;
	struct dvd_playback dvd_playback;
	char dvd_mpv_args[13] = {'\0'};
	mpv_handle *dvd_mpv = NULL;
	mpv_event *dvd_mpv_event = NULL;
	struct mpv_event_log_message *dvd_mpv_log_message = NULL;
	const char *home_dir = getenv("HOME");
	const char *lang = getenv("LANG");

	// Video Title Set
	struct dvd_vts dvd_vts[99];

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
	snprintf(dvd_playback.mpv_chapters_range, 8, "%u-%u", 1, 99);
	memset(dvd_playback.audio_lang, '\0', sizeof(dvd_playback.audio_lang));
	if(strlen(lang) >= 2)
		snprintf(dvd_playback.audio_lang, 3, "%s", strndup(lang, 2));
	memset(dvd_playback.audio_aid, '\0', sizeof(dvd_playback.audio_aid));
	memset(dvd_playback.subtitles_lang, '\0', sizeof(dvd_playback.subtitles_lang));
	if(strlen(lang) >= 2)
		snprintf(dvd_playback.subtitles_lang, 3, "%s", strndup(lang, 2));
	memset(dvd_playback.subtitles_sid, '\0', sizeof(dvd_playback.subtitles_sid));

	const char str_options[] = "Aa:c:dfhpSs:t:Vvwz";
	struct option long_options[] = {

		{ "track", required_argument, 0, 't' },
		{ "chapters", required_argument, 0, 'c' },
		{ "fullscreen", no_argument, 0, 'f' },
		{ "deinterlace", no_argument, 0, 'd' },
		{ "alang", required_argument, 0, 'a' },
		{ "slang", required_argument, 0, 's' },
		{ "aid", required_argument, 0, 'A' },
		{ "sid", required_argument, 0, 'S' },
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },
		{ "widescreen", no_argument, 0, 'w' },
		{ "pan-scan", no_argument, 0, 'p' },
		{ "verbose", no_argument, 0, 'v' },
		{ "debug", no_argument, 0, 'z' },
		{ 0, 0, 0, 0 }

	};

	int long_index = 0;
	int opt = 0;
	opterr = 1;
	char *token = NULL;

	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {

		switch(opt) {

			case 'A':
				strncpy(dvd_playback.audio_aid, optarg, 3);
				break;

			case 'a':
				strncpy(dvd_playback.audio_lang, optarg, 2);
				break;

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
				if(arg_first_chapter > arg_last_chapter)
					arg_first_chapter = arg_last_chapter;

				break;

			case 'd':
				dvd_playback.deinterlace = true;
				break;

			case 'f':
				dvd_playback.fullscreen = true;
				break;

			case 'h':
				print_usage(DVD_INFO_PROGRAM);
				return 0;

			case 'p':
				opt_pan_scan = true;
				break;

			case 's':
				strncpy(dvd_playback.subtitles_lang, optarg, 2);
				dvd_playback.subtitles = true;
				break;

			case 'S':
				strncpy(dvd_playback.subtitles_sid, optarg, 3);
				dvd_playback.subtitles = true;
				break;

			case 't':
				opt_track_number = true;
				arg_track_number = (uint16_t)strtoumax(optarg, NULL, 0);
				break;

			case 'V':
				print_version("dvd_player");
				return 0;

			case 'v':
				verbose = true;
				break;

			case 'w':
				opt_widescreen = true;
				break;

			case 'z':
				verbose = true;
				debug = true;
				break;

			// ignore unknown arguments
			case '?':
				print_usage("dvd_player");
				return 1;

			// let getopt_long set the variable
			case 0:
			default:
				break;

		}

	}

	if(opt_pan_scan && opt_widescreen)
		opt_pan_scan = false;
	
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
		fprintf(stderr, "dvd_player: error opening %s\n", device_filename);
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
		fprintf(stderr, "dvd_player: Invalid track number %d\n", arg_track_number);
		fprintf(stderr, "dvd_player: Valid track numbers: 1 to %u\n", dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	} else if(opt_track_number) {
		dvd_playback.track = arg_track_number;
	}

	uint16_t ix = 0;
	uint16_t track = 1;
	
	uint32_t longest_msecs = 0;
	uint16_t longest_widescreen_track = 0;
	uint32_t longest_widescreen_msecs = 0;
	uint16_t longest_pan_scan_track = 0;
	uint32_t longest_pan_scan_msecs = 0;
	
	for(ix = 0, track = 1; ix < dvd_info.tracks; ix++, track++) {
 
		vts = dvd_vts_ifo_number(vmg_ifo, ix + 1);
		vts_ifo = vts_ifos[vts];
		dvd_track_info(&dvd_tracks[ix], track, vmg_ifo, vts_ifo);

		if(dvd_tracks[ix].msecs > longest_msecs) {
			dvd_info.longest_track = track;
			longest_msecs = dvd_tracks[ix].msecs;
		}

		if(dvd_track_aspect_ratio_16x9(vts_ifo) && dvd_tracks[ix].msecs > longest_widescreen_msecs) {
			longest_widescreen_msecs = dvd_tracks[ix].msecs;
			longest_widescreen_track = track;
		}

		else if(dvd_track_aspect_ratio_4x3(vts_ifo) && dvd_tracks[ix].msecs > longest_pan_scan_msecs) {
			longest_pan_scan_msecs = dvd_tracks[ix].msecs;
			longest_pan_scan_track = track;
		}

	}

	if(opt_widescreen && longest_widescreen_track != 0)
		dvd_info.longest_track = longest_widescreen_track;
	else if(opt_pan_scan && longest_pan_scan_track != 0)
		dvd_info.longest_track = longest_pan_scan_track;

	// TODO if there is another track that is active, within ~1 seconds of longest track, and
	// the first is pan & scan, and the second is widescreen, switch to the widescreen one.
	// A more intelligent search might also see if the second one has audio tracks. Need to
	// find a reference DVD.

	// Set the track number to play if none is passed as an argument
	if(!opt_track_number)
		dvd_playback.track = dvd_info.longest_track;
	
	dvd_track = dvd_tracks[dvd_playback.track - 1];

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
	
	/**
	 * File descriptors and filenames
	 */
	dvd_file_t *dvdread_vts_file = NULL;

	vts = dvd_vts_ifo_number(vmg_ifo, dvd_playback.track);
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

		DVDCloseFile(dvdread_vts_file);

		if(vts_ifo)
			ifoClose(vts_ifo);

		if(vmg_ifo)
			ifoClose(vmg_ifo);

		if(dvdread_dvd)
			DVDClose(dvdread_dvd);

		return 1;

	}

	// DVD playback using libmpv
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

	// mpv zero-indexes tracks
	snprintf(dvd_mpv_args, 13, "dvdread://%u", dvd_playback.track - 1);

	// MPV uses zero-indexing for tracks, dvd_info uses one instead
	const char *dvd_mpv_commands[] = { "loadfile", dvd_mpv_args, NULL };

	// Load user's mpv configuration in ~/.config/dvd_player/mpv.conf (and friends)
	if(strlen(dvd_player.mpv_config_dir) > 0) {
		mpv_set_option_string(dvd_mpv, "config-dir", dvd_player.mpv_config_dir);
		mpv_set_option_string(dvd_mpv, "config", "yes");
	}

	// When choosing a chapter range, mpv will add 1 to the last one requested
	snprintf(dvd_playback.mpv_chapters_range, 8, "%u-%u", dvd_playback.first_chapter, dvd_playback.last_chapter + 1);

	// Playback options and default configuration
	mpv_set_option_string(dvd_mpv, "dvd-device", device_filename);
	if(strlen(dvd_info.title) > 0)
		mpv_set_option_string(dvd_mpv, "title", dvd_info.title);
	else
		mpv_set_option_string(dvd_mpv, "title", "dvd_player");
	mpv_set_option_string(dvd_mpv, "chapter", dvd_playback.mpv_chapters_range);
	mpv_set_option_string(dvd_mpv, "input-default-bindings", "yes");
	mpv_set_option_string(dvd_mpv, "input-vo-keyboard", "yes");
	if(strlen(dvd_playback.audio_aid) > 0)
		mpv_set_option_string(dvd_mpv, "aid", dvd_playback.audio_aid);
	else if(strlen(dvd_playback.audio_lang) > 0)
		mpv_set_option_string(dvd_mpv, "alang", dvd_playback.audio_lang);
	if(dvd_playback.subtitles && strlen(dvd_playback.subtitles_sid) > 0)
		mpv_set_option_string(dvd_mpv, "sid", dvd_playback.subtitles_sid);
	else if(dvd_playback.subtitles && strlen(dvd_playback.subtitles_lang) > 0)
		mpv_set_option_string(dvd_mpv, "slang", dvd_playback.subtitles_lang);
	if(dvd_playback.fullscreen)
		mpv_set_option_string(dvd_mpv, "fullscreen", NULL);
	if(dvd_playback.deinterlace)
		mpv_set_option_string(dvd_mpv, "deinterlace", "yes");
	if(opt_no_video)
		mpv_set_option_string(dvd_mpv, "video", "no");
	if(opt_no_audio)
		mpv_set_option_string(dvd_mpv, "audio", "no");

	// start mpv
	mpv_initialize(dvd_mpv);
	mpv_command(dvd_mpv, dvd_mpv_commands);

	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		// Goodbye :)
		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN || dvd_mpv_event->event_id == MPV_EVENT_END_FILE)
			break;

		if(debug && dvd_mpv_event->event_id != MPV_EVENT_LOG_MESSAGE)
			printf("dvd_player [mpv_event_name]: %s\n", mpv_event_name(dvd_mpv_event->event_id));

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

void print_usage(char *binary) {

	printf("%s - a DVD player using libmpv\n", binary);
	printf("\n");
	printf("Usage:\n");
	printf("  dvd_player [dvd path] [options]\n");
	printf("\n");
	printf("Display options:\n");
	printf("  -f, --fullscreen              Display in fullscreen mode\n");
	printf("  -d, --deinterlace             Deinterlace video\n");
	printf("\n");
	printf("Track selection:\n");
	printf("  -t, --track <#>               Select DVD track number (default: longest)\n");
	printf("  -w, --widescreen              Select longest widescreen track\n");
	printf("  -p, --pan-scan                Select longest pan & scan track\n");
	printf("  -c, --chapters                Select chapter(s) range (default: all)\n");
	printf("        {start|start-end|-end}\n");
	printf("  -a, --alang <language>        Select audio language, two character code (default: first audio track)\n");
	printf("  -A, --aid <##>                Select audio track ID\n");
	printf("  -s, --slang <language>        Select subtitles language, two character code (default: no subtitles)\n");
	printf("  -S, --sid <##>                Select subtitles track ID\n");
	printf("\n");
	printf("Extra information:\n");
	printf("  -v, --verbose                 Verbose output\n");
	printf("  -z, --debug                   Debugging output\n");
	printf("\n");
	printf("Executable options:\n");
	printf("  -h, --help			Show this help text and exit\n");
	printf("  -V, --version                 Show version info and exit\n");
	printf("\n");
	printf("DVD Path Examples:\n");
	printf("  dvd_player                    Play back default DVD device - %s\n", DEFAULT_DVD_DEVICE);
	printf("  dvd_player /dev/dvd           Play back a device name\n");
	printf("  dvd_player DVD.iso            Play back a DVD image file\n");
	printf("  dvd_player DVD/               Play back a DVD directory containing VIDEO_TS/ subdirectory\n");
	printf("\n");
	printf("dvd_player reads a configuration file from ~/.config/dvd_player/mpv.conf\n");
	printf("See mpv man page for syntax or dvd_player man page for examples.\n");

}

void print_version(char *binary) {

	printf("%s %s\n", binary, DVD_INFO_VERSION);

}
