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
#include <inttypes.h>
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
#ifndef VERSION
#define VERSION "1.3"
#endif

#define DVD_INFO_PROGRAM "dvd_player"

int main(int, char **);
void dvd_track_info(struct dvd_track *dvd_track, const uint16_t track_number, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo);
void print_usage(char *binary);
void print_version(char *binary);

struct dvd_player {
	uint16_t track;
	uint8_t first_chapter;
	uint8_t last_chapter;
	uint8_t first_cell;
	uint8_t last_cell;
};

struct dvd_playback {
	bool fullscreen;
	bool deinterlace;
	char audio_lang[3];
	char subs_lang[3];
	uint8_t audio_track;
	uint8_t subtitle_track;
	char mpv_aid[3];
	char mpv_sid[3];
};

int main(int argc, char **argv) {

	/**
	 * Parse options
	 */

	bool opt_track_number = false;
	bool opt_chapter_number = false;
	bool opt_filename = false;
	bool opt_audio = false;
	bool opt_subs = false;
	uint16_t arg_track_number = 0;
	uint8_t active_audio_tracks = 0;
	uint8_t audio_track = 0;
	uint8_t audio_tracks =  0;
	uint8_t active_subtitle_tracks = 0;
	uint8_t subtitle_tracks = 0;
	uint8_t subtitle_track = 0;
	char audio_track_lang[DVD_AUDIO_LANG_CODE + 1] = {'\0'};
	char subtitle_track_lang[DVD_SUBTITLE_LANG_CODE + 1] = {'\0'};
	int long_index = 0;
	int opt = 0;
	opterr = 1;
	const char *str_options;
	str_options = "a:c:dfhs:t:V";
	uint8_t arg_first_chapter = 1;
	uint8_t arg_last_chapter = 99;
	char *token = NULL;
	struct dvd_player dvd_player;
	struct dvd_playback dvd_playback;
	char opt_audio_lang[3] = {'\0'};
	char opt_subs_lang[3] = {'\0'};
	char dvd_mpv_args[13] = {'\0'};
	char dvd_mpv_first_chapter[4] = {'\0'};
	char dvd_mpv_last_chapter[4] = {'\0'};
	mpv_handle *dvd_mpv;
	mpv_event *dvd_mpv_event;

	dvd_player.track = 1;
	dvd_player.first_chapter = 1;
	dvd_player.last_chapter = 99;
	dvd_player.first_cell = 1;
	dvd_player.last_cell = 1;

	dvd_playback.fullscreen = false;
	dvd_playback.deinterlace = false;
	snprintf(dvd_playback.audio_lang, 3, "en");
	snprintf(dvd_playback.subs_lang, 3, "en");
	dvd_playback.audio_track = 0;
	dvd_playback.subtitle_track = 0;
	snprintf(dvd_playback.mpv_aid, 3, "no");
	snprintf(dvd_playback.mpv_sid, 3, "no");

	struct option long_options[] = {

		{ "audio", required_argument, 0, 'a' },
		{ "chapters", required_argument, 0, 'c' },
		{ "deinterlace", no_argument, 0, 'd' },
		{ "fullscreen", no_argument, 0, 'f' },
		{ "subtitles", required_argument, 0, 's' },
		{ "track", required_argument, 0, 't' },
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },
		{ 0, 0, 0, 0 }

	};

	while((opt = getopt_long(argc, argv, str_options, long_options, &long_index )) != -1) {

		switch(opt) {

			case 'a':
				opt_audio = true;
				strncpy(opt_audio_lang, optarg, 2);
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

			case 's':
				opt_subs = true;
				strncpy(opt_subs_lang, optarg, 2);
				break;

			case 't':
				opt_track_number = true;
				arg_track_number = (uint16_t)strtoumax(optarg, NULL, 0);
				break;

			case 'V':
				print_version("dvd_player");
				return 0;

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
		fprintf(stderr, "dvd_player: Invalid track number %d\n", arg_track_number);
		fprintf(stderr, "dvd_player: Valid track numbers: 1 to %u\n", dvd_info.tracks);
		ifoClose(vmg_ifo);
		DVDClose(dvdread_dvd);
		return 1;
	} else if(opt_track_number) {
		dvd_player.track = arg_track_number;
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

	// TODO if there is another track that is active, within ~1 seconds of longest track, and
	// the first is pan & scan, and the second is widescreen, switch to the widescreen one.
	// A more intelligent search might also see if the second one has audio tracks. Need to
	// find a reference DVD.

	// Set the track number to play if none is passed as an argument
	if(!opt_track_number)
		dvd_player.track = dvd_info.longest_track;
	
	dvd_track = dvd_tracks[dvd_player.track - 1];

	// Set the proper chapter range
	if(opt_chapter_number) {
		if(arg_first_chapter > dvd_track.chapters) {
			dvd_player.first_chapter = dvd_track.chapters;
			fprintf(stderr, "Resetting first chapter to %u\n", dvd_player.first_chapter);
		} else
			dvd_player.first_chapter = arg_first_chapter;
		
		if(arg_last_chapter > dvd_track.chapters) {
			dvd_player.last_chapter = dvd_track.chapters;
			fprintf(stderr, "Resetting last chapter to %u\n", dvd_player.last_chapter);
		} else
			dvd_player.last_chapter = arg_last_chapter;
	} else {
		dvd_player.first_chapter = 1;
		dvd_player.last_chapter = dvd_track.chapters;
	}
	
	/**
	 * File descriptors and filenames
	 */
	dvd_file_t *dvdread_vts_file = NULL;

	vts = dvd_vts_ifo_number(vmg_ifo, dvd_player.track);
	vts_ifo = vts_ifos[vts];

	// Open the VTS VOB
	dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts, DVD_READ_TITLE_VOBS);

	printf("Track: %02u, Length: %s, Chapters: %02u, Cells: %02u, Audio streams: %02u, Subpictures: %02u, Filesize: %lu, Blocks: %lu\n", dvd_track.track, dvd_track.length, dvd_track.chapters, dvd_track.cells, dvd_track.audio_tracks, dvd_track.subtitles, dvd_track.filesize, dvd_track.blocks);

	dvd_player.first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, dvd_player.track, dvd_player.first_chapter);
	dvd_player.last_cell = dvd_chapter_last_cell(vmg_ifo, vts_ifo, dvd_player.track, dvd_player.last_chapter);

	// Audio track selection based on user input and searching
	
	// If only one track, stop here
	/** Audio track selection **/
	// Start with no audio track selected, but loop through all the tracks and then select
	// the first active one. If an audio language is specified, loop through all the
	// active audio tracks, and choose the first that matches the audio language.
	// Some DVDs have audio tracks listed, but none that are active, so this will skip
	// the auto select inside of libmpv.
	// TODO if all the languages are the same, choose the one with the highest number of
	// chapters, which is most likely to be the main audio track. (add in if I can find a
	// DVD that actually has that)
	// TODO this selects the first audio language on the DVD, whether it is English or not;
	// I can't check each track to see if it is 'en' or not, since some may not be labelled
	// with a language at all -- 'un' for undefined. In those cases, need to decide what to
	// do. Need to find a DVD sample that does this.
	// TODO add option to prefer DTS over AC3?
	// TODO capture mpv event of audio track changing 'tracks-changed' and work around

	active_audio_tracks = dvd_audio_active_tracks(vmg_ifo, vts_ifo, dvd_player.track);

	if(active_audio_tracks) {
		
		audio_tracks = dvd_track_audio_tracks(vts_ifo);

		for(audio_track = 1; audio_track < audio_tracks + 1; audio_track++) {

			if(!opt_audio && dvd_playback.audio_track == 0 && dvd_audio_active(vmg_ifo, vts_ifo, dvd_player.track, audio_track)) {
				dvd_playback.audio_track = audio_track;
				break;
			}

			if(!dvd_audio_active(vmg_ifo, vts_ifo, dvd_player.track, audio_track))
				continue;
			
			if(opt_audio && dvd_playback.audio_track == 0) {
				dvd_audio_lang_code(audio_track_lang, vts_ifo, audio_track - 1);
				printf("dvd_player: audio track lang: %s\n", audio_track_lang);
				printf("dvd_player: opt audio track: %s\n", opt_audio_lang);
				if(strncmp(audio_track_lang, opt_audio_lang, 2) == 0) {
					printf("dvd_player: found audio track %d\n", audio_track);
					dvd_playback.audio_track = audio_track;
					break;
				}
			}

		}
		
	}
	printf("dvd_player: playback audio track: %u\n", dvd_playback.audio_track);
	
	// Translate the audio track into audio ID for mpv
	// dvd_playback.mpv_aid = 127 + dvd_playback.audio_track;
	snprintf(dvd_playback.mpv_aid, 3, "%d", dvd_playback.audio_track);
	printf("dvd_player: dvd_playback.mpv_aid: %s\n", dvd_playback.mpv_aid);

	/** Subtitle track selection **/
	// Looking for a subtitle track is the same as audio, except that it does not display
	// by default at all, and will only look at them if asked to be enabled.
	// TODO capture an mpv event for changing subtitle tracks (track-switched), and switch
	// to the first subtitle track of the same audio language.
	// TODO capture mpv event of subtitle track changing 'tracks-changed' and work around
	// inactive subtitle tracks.

	active_subtitle_tracks = dvd_track_active_subtitles(vmg_ifo, vts_ifo, dvd_player.track);

	if(active_subtitle_tracks && opt_subs) {
		
		subtitle_tracks = dvd_track_subtitles(vts_ifo);

		for(subtitle_track = 1; subtitle_track < subtitle_tracks + 1; subtitle_track++) {

			if(!dvd_subtitle_active(vmg_ifo, vts_ifo, dvd_player.track, subtitle_track))
				continue;

			if(opt_subs && dvd_playback.subtitle_track == 0) {

				dvd_subtitle_lang_code(subtitle_track_lang, vts_ifo, subtitle_track - 1);
				printf("dvd_player: subtitle track lang: %s\n", subtitle_track_lang);
				printf("dvd_player: opt audio track: %s\n", opt_subs_lang);
				if(strncmp(subtitle_track_lang, opt_subs_lang, 2) == 0) {
					printf("dvd_player: found subtitle track: %d\n", subtitle_track);
					dvd_playback.subtitle_track = subtitle_track;
					break;
				}

			}
			
		}
	}

	snprintf(dvd_playback.mpv_sid, 3, "%d", dvd_playback.subtitle_track);
	printf("dvd_player: dvd_playback.mpv_sid: %s\n", dvd_playback.mpv_sid);

	// DVD playback using libmpv
	dvd_mpv = mpv_create();

	// mpv zero-indexes tracks
	sprintf(dvd_mpv_args, "dvdread://%02u", dvd_player.track - 1);

	// mpv's chapter range starts at the first one, and ends at the last one plus one
	// fex: to play chapter 1 only, mpv --start '#1' --end '#2'
	sprintf(dvd_mpv_first_chapter, "#%02u", dvd_player.first_chapter);
	sprintf(dvd_mpv_last_chapter, "#%02u", dvd_player.last_chapter + 1);

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
	mpv_set_option_string(dvd_mpv, "aid", dvd_playback.mpv_aid);
	mpv_set_option_string(dvd_mpv, "sid", dvd_playback.mpv_sid);
	mpv_set_option_string(dvd_mpv, "input-default-bindings", "yes");
	mpv_set_option_string(dvd_mpv, "input-vo-keyboard", "yes");
	mpv_set_option_string(dvd_mpv, "resume-playback", "no");
	mpv_set_option_string(dvd_mpv, "fullscreen", NULL);
	mpv_set_option_string(dvd_mpv, "deinterlace", "yes");

	mpv_initialize(dvd_mpv);
	mpv_command(dvd_mpv, dvd_mpv_commands);

	// https://github.com/mpv-player/mpv/blob/master/libmpv/client.h#L622
	char *mpv_current_subtitle;
	char mpv_new_subtitle[3] = {'\0'};
	int num_changes = 0;

	while(true) {

		dvd_mpv_event = mpv_wait_event(dvd_mpv, -1);

		printf("mpv_event_name: %s\n", mpv_event_name(dvd_mpv_event->event_id));

		if(dvd_mpv_event->event_id == MPV_EVENT_SHUTDOWN || dvd_mpv_event->event_id == MPV_EVENT_END_FILE)
			break;

		// TODO limit switching subtitles to languages specified
		// TODO this code avoids an endless loop, just need to actually change them
		/*
		if(dvd_mpv_event->event_id == MPV_EVENT_TRACK_SWITCHED) {
			num_changes++;
			printf("num_changes: %i\n", num_changes);
			if(num_changes == 2) {
				num_changes = 0;
				continue;
			}
			mpv_get_property(dvd_mpv, "sid", MPV_FORMAT_STRING, &mpv_current_subtitle);
			printf("CURRENT MPV SUBTITLE: %s\n", mpv_current_subtitle);
			printf("Changing tracks (audio or subtitle)\n");
			snprintf(mpv_new_subtitle, 3, "no");
			printf("NEW MPV SUBTITLE: %s\n", mpv_new_subtitle);
			mpv_set_option_string(dvd_mpv, "sid", mpv_new_subtitle);
			continue;
		}
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

	printf("%s %s - play a DVD track\n", binary, VERSION);
	printf("\n");
	printf("Usage: %s [-t track] [-c chapter[-chapter]] [dvd path] [options]\n", binary);
	printf("\n");
	printf("Options:\n");
	printf(" -f, --fullscreen\n");
	printf(" -d, --deinterlace\n");
	printf("\n");
	printf("DVD path can be a device name, a single file, or directory.\n");
	printf("\n");
	printf("Examples:\n");
	printf("  dvd_player			# Read default DVD device (%s)\n", DEFAULT_DVD_DEVICE);
	printf("  dvd_player /dev/dvd		# Read a specific DVD device\n");
	printf("  dvd_player video.iso    	# Read an image file\n");
	printf("  dvd_player ~/Videos/DVD	# Read a directory that contains VIDEO_TS\n");
	printf("\n");

}

void print_version(char *binary) {

	printf("%s %s - http://dvds.beandog.org/ - (c) 2018 Steve Dibb <steve.dibb@gmail.com>, licensed under GPL-2\n", binary, VERSION);

}
