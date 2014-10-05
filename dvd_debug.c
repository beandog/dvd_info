#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <inttypes.h>
#include <linux/cdrom.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/dvd_udf.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>
#include <dvdnav/dvdnav.h>
#include "dvd_debug_ifo.h"
#include "dvd_vmg_ifo.h"
#include "dvd_track.h"

#define DEFAULT_DVD_DEVICE "/dev/dvd"

struct dvd_info {
	uint16_t video_title_sets;
	uint8_t side;
	char title[33];
	char provider_id[33];
	char vmg_id[13];
	uint16_t tracks;
	uint16_t longest_track;
};

struct dvd_track {
	int number;
	int title_idx;
	uint8_t vts;
	char length[13];
	uint8_t chapters;
	uint8_t audio_tracks;
	uint8_t subtitles;
	uint8_t cells;
};

struct dvd_video {
	char codec[6];
	char format[5];
	char aspect_ratio[5];
	uint16_t width;
	uint16_t height;
	bool letterbox;
	bool pan_and_scan;
};

struct dvd_audio {
	int track;
	int stream;
	char lang_code[3];
	char codec[5];
	int channels;
};

struct dvd_subtitle {
	int track;
	int stream;
	char lang_code[3];
};

void d_test(const char *str) {

	printf("%s: ", str);

}

void d_str(const char *str) {

	printf("%s\n", str);

}

void d_pass() {

	printf("pass\n");

}

void d_fail() {

	printf("fail\n");

}

int main(int argc, char **argv) {

	// dvd_debug
	uint16_t track_number = 0;
	uint16_t vts = 1;
	int retval;
	ssize_t dvd_filesize;
	unsigned char *buffer = NULL;
	ifo_handle_t *debug_ifo = NULL;
	ssize_t bytes_read;

	// Device hardware
	int dvd_fd;
	const char *device_filename = DEFAULT_DVD_DEVICE;
	bool is_hardware = false;
	bool is_image = false;

	// libdvdread
	dvd_reader_t *dvdread_dvd;
	ifo_handle_t *vmg_ifo = NULL;
	ifo_handle_t *vts_ifo = NULL;
	unsigned char dvdread_ifo_md5[16] = {'\0'};
	char dvdread_id[33] = {'\0'};
	uint8_t vts_ttn;
	pgc_t *pgc;
	pgcit_t *vts_pgcit;

	// DVD
	struct dvd_info dvd_info;
	dvd_info.video_title_sets = 1;
	dvd_info.side = 1;
	memset(dvd_info.title, '\0', 33);
	memset(dvd_info.provider_id, '\0', 33);
	memset(dvd_info.vmg_id, '\0', 13);
	dvd_info.tracks = 1;
	dvd_info.longest_track = 1;

	// Track
	struct dvd_track dvd_track;
	dvd_track.number = 1;
	dvd_track.title_idx = 0;
	dvd_track.vts = 1;
	memset(dvd_track.length, '\0', 13);
	dvd_track.chapters = 1;
	dvd_track.audio_tracks = 0;
	dvd_track.subtitles = 0;
	dvd_track.cells = 1;

	// Video
	struct dvd_video dvd_video;
	memset(dvd_video.codec, '\0', 6);
	memset(dvd_video.format, '\0', 5);
	memset(dvd_video.aspect_ratio, '\0', 5);
	dvd_video.width = 0;
	dvd_video.height = 0;
	dvd_video.letterbox = false;
	dvd_video.pan_and_scan = false;

	// Audio
	struct dvd_audio dvd_audio;
	uint8_t stream;
	dvd_audio.track = 1;
	dvd_audio.stream = 0;
	memset(dvd_audio.lang_code, '\0', 3);
	memset(dvd_audio.codec, '\0', 5);
	dvd_audio.channels = 0;

	// Subtitles
	struct dvd_subtitle dvd_subtitle;
	dvd_subtitle.track = 1;
	dvd_subtitle.stream = 0;
	memset(dvd_subtitle.lang_code, '\0', 3);

	// Chapters
	uint8_t chapter_number;
	char chapter_length[14] = {'\0'};

	bool has_bugs = false;
	uint8_t bugs = 0;
	uint8_t anomalies = 0;

	d_str("[DVD Debug]");

	// If '-i /dev/device' is not passed, then set it to the string
	// passed.  fex: 'dvd_info /dev/dvd1' would change it from the default
	// of '/dev/dvd'.
	if (argv[optind]) {
		device_filename = argv[optind];
	}

	d_test("device filename");
	d_str(device_filename);

	d_test("access device");

	// Check to see if device can be accessed
	if(access(device_filename, F_OK) == 0)
		d_pass();
	else {
		d_fail();
		return 0;
	}

	// Check to see if device can be opened
	d_test("open device");
	dvd_fd = open(device_filename, O_RDONLY | O_NONBLOCK);
	if(dvd_fd == 0) {
		d_pass();
	} else {
		d_fail();
	}

	// Check to see if device can be closed
	if(dvd_fd == 0) {
		d_test("close device");
		if(close(dvd_fd) == 0)
			d_pass();
		else {
			d_fail();
		}
	}

	// Open DVD device with libdvdread
	d_test("DVDOpen()");
	dvdread_dvd = DVDOpen(device_filename);
	if(dvdread_dvd) {
		d_pass();
	} else {
		d_fail();
		return 1;
	}

	// TODO: Add checks for IFO filesizes and content, and report if they
	// match or not.

	// Testing - Open IFO directly
	// See DVDCopyIfoBup() in dvdbackup.c for reference
	/*
	dvd_read_domain_t *dvd_read_domain;
	dvd_read_domain = DVD_READ_INFO_FILE;
	dvd_file_t *dvd_file;
	dvd_file = DVDOpenFile(dvdread_dvd, 1, DVD_READ_INFO_FILE);
	if(dvd_file == 0) {
		printf("* DVDOpenFile() failed\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Get the file size in kilobytes
	dvd_filesize = DVDFileSize(dvd_file) * DVD_VIDEO_LB_LEN;
	DVDCloseFile(dvd_file);
	// printf("* Block filesize: %ld\n", dvd_filesize);
	debug_ifo = ifoOpen(dvdread_dvd, 1);
	// What is ifo->file ? It's a dvd_file_t variable
	DVDFileSeek(debug_ifo->file, 0);
	// Allocate enough memory for the buffer, *now that we know the filesize*
	// buffer = (unsigned char *)malloc((unsigned long)dvd_filesize * sizeof(unsigned char));
	// diff between malloc & calloc? http://stackoverflow.com/questions/1538420/difference-between-malloc-and-calloc
	// buffer = (unsigned char *)calloc((unsigned long)dvd_filesize, sizeof(unsigned char));
	// Need to check to make sure it could read the right size
	// bytes_read = DVDReadBytes(debug_ifo->file, buffer, (size_t)dvd_filesize);
	ifoClose(debug_ifo);
	if(bytes_read != dvd_filesize) {
		printf("* bytes read and dvd_filesize do not match: %ld, %ld\n", bytes_read, dvd_filesize);
		has_bugs = true;
	}
	// Need to free manually allocated memory, releasing the pointer
	free(buffer);
	*/

	// Open IFO zero -- where all the cool stuff is
	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	// FIXME do a proper exit
	if(!vmg_ifo) {
		printf("* Opening IFO zero failed\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	// Total # of IFOs
	dvd_info.video_title_sets = vmg_ifo->vts_atrt->nr_of_vtss;
	bool valid_ifos[dvd_info.video_title_sets];

	// Access the VMGIT (Video Manager Information Management Table)
	vmgi_mat_t *vmgi_mat;
	vmgi_mat = vmg_ifo->vmgi_mat;

	// printf("Video Manager Information Management Table:\n");
	printf("[VMG]\n");
	printf("* VMGI MAT Title Sets (# IFOs): %d\n", vmgi_mat->vmg_nr_of_title_sets);
	printf("* IFO 0 Title Sets (# IFOS): %d\n", vmg_ifo->vmgi_mat->vmg_nr_of_title_sets);
	printf("* IFO 0 Title Tracks (aka tracks): %d\n", vmg_ifo->tt_srpt->nr_of_srpts);

	// VMG: Disc Side (1, 2)
	// Check for invalid disc side
	if(vmg_ifo->vmgi_mat->disc_side > 2) {
		printf("* VMG FAIL: Invalid disc side: %i\n", vmg_ifo->vmgi_mat->disc_side);
		bugs++;
	}

	// VMG: # menu audio streams
	// Check for invalid number of audio streams in menu VOBs
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->nr_of_vmgm_audio_streams > 1) {
		printf("* VMG FAIL: Invalid num. of audio streams: %i\n", vmg_ifo->vmgi_mat->nr_of_vmgm_audio_streams);
		bugs++;
	}

	// VMG: Video attributes of menu VOBs
	// VMG Video: MPEG version
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.mpeg_version > 1) {
		printf("* VMG FAIL: Menu video codec is invalid: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.mpeg_version);
		bugs++;
	}
	// VMG Video: NTSC / PAL
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.video_format > 1) {
		printf("* VMG FAIL: Invalid video format: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.video_format);
		bugs++;
	}
	// VMG Video: Aspect Ratio
	// Valid values: 0 or 3 (4:3 and 16:9, respectively)
	if((vmg_ifo->vmgi_mat->vmgm_video_attr.display_aspect_ratio != 0) && (vmg_ifo->vmgi_mat->vmgm_video_attr.display_aspect_ratio != 3)) {
		printf("* VMG FAIL: Invalid aspect ratio: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.display_aspect_ratio);
		bugs++;
	}
	// VMG Video: Letterbox / Pan and Scan
	// **README** I don't understand what these values are very well.
	// The dvdinfo page ( http://stnsoft.com/DVD/ifo.html#vidatt ) looks like
	// there is one byte for each value, which can be toggled.  libdvdread,
	// however, indicates that there is two bytes, but the valid values are
	// between 0 and 3.  This isn't the first time the two disagree on order
	// or valid values (see unknown1 for video attributes).  In this case,
	// though, I'm going with libdvdread as the valid reference, and will
	// back up my findings with examining IFOs en masse.  It's worth
	// reviewing the data on the DVD directly, and seeing what is in those
	// two bytes if I looked at them individually.
	// Valid values (from libdvdread): 0, 1, 2, 3
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.permitted_df > 3) {
		printf("* VMGI FAIL: Invalid display format: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.permitted_df);
		bugs++;
	}
	// VMG Video: Invalid NTSC CC lines
	// An NTSC video can have lines set for field 1 and field 2.  Check
	// here if they are set while video is not NTSC
	if((vmg_ifo->vmgi_mat->vmgm_video_attr.video_format != 0) && (vmg_ifo->vmgi_mat->vmgm_video_attr.line21_cc_1 != 0)) {
		printf("* VMG FAIL: Line 21 CC 1 is set on non-NTSC video\n");
		bugs++;
	}
	if((vmg_ifo->vmgi_mat->vmgm_video_attr.video_format != 0) && (vmg_ifo->vmgi_mat->vmgm_video_attr.line21_cc_2 != 0)) {
		printf("* VMG FAIL: Line 21 CC 2 is set on non-NTSC video\n");
		bugs++;
	}
	// VMG Video: Invalid video resolution check
	// Valid values: 0, 1, 2, 3
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.picture_size > 3) {
		printf("* VMG FAIL: Invalid video format: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.picture_size);
		bugs++;
	}
	// VMG Video: Invalid letterbox check
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.letterboxed > 1) {
		printf(" VMG FAIL: Invalid letterbox value: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.letterboxed);
		bugs++;
	}
	// VMG Video: Invalid film type check
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.film_mode > 1) {
		printf("* VMG FAIL: Invalid film mode: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.film_mode);
		bugs++;
	}

	// VMG Audio: check number of audio streams
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->nr_of_vmgm_audio_streams > 1) {
		printf("* VMG FAIL: Invalid number of audio streams: %i\n", vmg_ifo->vmgi_mat->nr_of_vmgm_audio_streams);
		bugs++;
	}
	// VMG Audio: check for valid audio format
	// Valid values: 0, 2, 3, 4, 6
	if(vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format == 1 ||
		vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format == 5 ||
		vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format > 6) {
		printf("* VMG FAIL: Invalid audio format number: %i\n", vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format);
		bugs++;
	}
	// VMG Audio: check for valid quantization
	// Valid values: 0, 1, 2, 3
	if(vmg_ifo->vmgi_mat->vmgm_audio_attr.quantization > 3) {
		printf("* VMG FAIL: Invalid audio quantization: %i\n", vmg_ifo->vmgi_mat->vmgm_audio_attr.quantization);
		bugs++;
	}
	// VMG Audio: check for valid sample frequency
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_audio_attr.sample_frequency > 1) {
		printf("* VMG FAIL: Invalid audio sample frequency: %i\n", vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format);
	}
	// VMG Audio: check for valid channels
	// Valid values: 0 through 6 (I've only seen 0, 1 and 5 so far)
	// Actual number of channels is 1 + this value (0 = mono, 1 = stereo)
	if(vmg_ifo->vmgi_mat->vmgm_audio_attr.channels > 5) {
		printf("* VMG ANOMALY: num audio channels for menu VOB is: %i\n", vmg_ifo->vmgi_mat->vmgm_audio_attr.channels);
		anomalies++;
	}

	// VMGM Subtitles: check for number of subp streams
	// Valid values: 0 (I think)
	if(vmg_ifo->vmgi_mat->vmgm_subp_attr.code_mode > 0) {
		printf("* VMG UNKNOWN: subpicture code mode is not zero: %i\n", vmg_ifo->vmgi_mat->vmgm_subp_attr.code_mode);
		anomalies++;
	}


	// Check for invalid IFOs
	for(vts = 1; vts < vmg_ifo->vts_atrt->nr_of_vtss + 1; vts++) {

		printf("[IFO %u]\n", vts);

		vts_ifo = ifoOpen(dvdread_dvd, vts);

		if(vts_ifo) {
			valid_ifos[vts] = true;
			ifoClose(vts_ifo);
			vts_ifo = NULL;
		} else {
			valid_ifos[vts] = false;
			printf("ifoOpen(%u) FAILED\n", vts);
			vts_ifo = NULL;
		}

	}

	// Title IFO todo list:
	// * Verify last sector of title set
	// * Verify last sector of IFO
	// * Verify all sector pointers

	// Title IFO: check for invalid VTS category
	// Valid values: 0, 1 (unspecified, karaoke)

	// COMMENTED OUT WHILE TESTING .. CODE IS GOOD TO GO

	/**

	if(vts_ifo->vtsi_mat->vts_category > 1) {
		printf("* VTS FAIL: Invalid VTS category: %i\n", vts_ifo->vtsi_mat->vts_category);
		bugs++;
	}
	// Title IFO Menu Video: check for invalid codec
	if(vts_ifo->vtsi_mat->vtsm_video_attr.mpeg_version > 1) {
		printf("* VTS Menu FAIL: Invalid video codec: %i\n", vts_ifo->vtsi_mat->vtsm_video_attr.mpeg_version);
		bugs++;
	}
	// Title IFO Menu Video: Standard format (NTSC, PAL)
	if(vts_ifo->vtsi_mat->vtsm_video_attr.video_format > 1) {
		printf("* VTS Menu FAIL: Invalid video format: %i\n", vts_ifo->vtsi_mat->vtsm_video_attr.video_format);
		bugs++;
	}
	// Title IFO Menu Video: Aspect Ratio
	// Valid values: 0 or 3 (4:3 and 16:9, respectively)
	if((vts_ifo->vtsi_mat->vtsm_video_attr.display_aspect_ratio != 0) && (vts_ifo->vtsi_mat->vtsm_video_attr.display_aspect_ratio != 3)) {
		printf("* VTS Menu FAIL: Invalid aspect ratio: %i\n", vts_ifo->vtsi_mat->vtsm_video_attr.display_aspect_ratio);
		bugs++;
	}
	// Title IFO Menu Video: Letterbox / Pan and Scan
	*/



	/** Old code **/

	// IFO ZERO ->  TITLE SECTOR POINTERS (srpts)
	// tt_srpt: http://stnsoft.com/DVD/ifo_vmg.html#tt
	/*
	num_title_tracks = vmg_ifo->tt_srpt->nr_of_srpts;
	int num_search_pointers;
	num_search_pointers = vmg_ifo->tt_srpt->nr_of_srpts;
	num_title_sets = vmg_ifo->vmgi_mat->vmg_nr_of_title_sets;
	*/

	// Warn if track number is higher than amount expected
	// libdvdread might throw this as a warning if accessing the
	// highest track number:
	// libdvdread: No VTS_TMAPT available - skipping.
	/*
	if(display_track && ((track_number > num_title_tracks) || (track_number > num_title_sets))) {
		printf("* WARNING: Track number %i is possibly invalid, calculating max number of tracks %i exist\n", track_number, max_tracks);
		bugs++;
	}
	*/

	/**
	 * Track information
	 */

	/*
	for(track_number = 1; track_number <= num_search_pointers; track_number++) {

		printf("[Track %i]\n", track_number);

		vts_ifo = ifoOpen(dvdread_dvd, ifo_number);
		if(!vts_ifo) {
			printf("Track %2d: opening IFO %d failed!\n", track_number, ifo_number);
			vts_ifo = NULL;
			bugs++;
			break;
		}

		vts_ttn = vmg_ifo->tt_srpt->title[track_number - 1].vts_ttn;
		vts_pgcit = vts_ifo->vts_pgcit;
		pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1].pgc;

		// Title track length (HH:MM:SS.MS)
		dvd_track_str_length(&pgc->playback_time, title_track_length);

		if(vts_ifo->vtsi_mat->vts_tmapt == 0 || vts_ifo->vtsi_mat->vts_tmapt == NULL) {
			printf("IFO %d: no VTS_TMAPT   Track length: %s\n", title_vts_ifo_number, title_track_length);
		}

		title_track_length[0] = '\0';

		// Chapters
		// TODO Check VTS IFO chapter amount against VMG IFO
		// vmg: vmg_ifo->tt_srpt->title[track_number].nr_of_ptts
		// vts: pgc->nr_of_programs

		ifoClose(vts_ifo);
		vts_ifo = NULL;

	}
	*/

	// Cleanup

	if(dvdread_dvd)
		DVDClose(dvdread_dvd);

	int e = 0;

	if(bugs)
		e = 1;

	if(anomalies)
		e = 2;

	if(bugs && anomalies)
		e = 3;

	return e;

}
