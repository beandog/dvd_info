#include "dvd_debug.h"

int dvd_debug(dvd_reader_t *dvdread_dvd) {

	ifo_handle_t *vmg_ifo = NULL;

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

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "dvd_debug: Opening VMG IFO failed\n");
		return 1;
	}

	ifo_handle_t *vts_ifo = NULL;
	uint16_t vts = 1;
	uint16_t title_tracks = dvd_tracks(vmg_ifo);
	uint16_t title_track = 1;
	uint32_t title_track_msecs = 0;
	uint8_t ttn = 1;
	pgcit_t *vts_pgcit = NULL;
	pgc_t *pgc = NULL;
	dvd_time_t *track_time = NULL;

	uint8_t chapters = 1;
	uint16_t nr_of_ptts = 1;
	uint8_t chapter = 1;
	uint32_t chapters_msecs = 0;
	uint32_t chapter_msecs = 0;

	uint8_t cells = 0;
	uint8_t cell = 0;
	uint32_t cells_msecs = 0;
	uint32_t cell_msecs = 0;

	bool length_mismatch = false;
	bool chapters_mismatch = false;

	// Total # of IFOs
	bool valid_ifos[DVD_MAX_VTS_IFOS];

	// Access the VMGIT (Video Manager Information Management Table)
	vmgi_mat_t *vmgi_mat;
	vmgi_mat = vmg_ifo->vmgi_mat;

	// printf("Video Manager Information Management Table:\n");
	printf("[VMG]\n");
	printf("* VMGI MAT Title Sets (# IFOs): %d\n", vmgi_mat->vmg_nr_of_title_sets);
	printf("* IFO 0 Title Sets (# IFOS): %d\n", vmg_ifo->vmgi_mat->vmg_nr_of_title_sets);
	printf("* IFO 0 Title Tracks (aka tracks): %d\n", vmg_ifo->tt_srpt->nr_of_srpts);
	printf("* Specification version: %01x.%01x\n", vmg_ifo->vmgi_mat->specification_version >> 4, vmg_ifo->vmgi_mat->specification_version & 0xf);

	// VMG: Disc Side (1, 2)
	// Check for invalid disc side
	if(vmg_ifo->vmgi_mat->disc_side > 2) {
		printf("* VMG FAIL: Invalid disc side: %i\n", vmg_ifo->vmgi_mat->disc_side);
	}

	// VMG: # menu audio streams
	// Check for invalid number of audio streams in menu VOBs
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->nr_of_vmgm_audio_streams > 1) {
		printf("* VMG FAIL: Invalid num. of audio streams: %i\n", vmg_ifo->vmgi_mat->nr_of_vmgm_audio_streams);
	}

	// VMG: Video attributes of menu VOBs
	// VMG Video: MPEG version
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.mpeg_version > 1) {
		printf("* VMG FAIL: Menu video codec is invalid: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.mpeg_version);
	}

	// VMG Video: NTSC / PAL
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.video_format > 1) {
		printf("* VMG FAIL: Invalid video format: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.video_format);
	}

	// VMG Video: Aspect Ratio
	// Valid values: 0 or 3 (4:3 and 16:9, respectively)
	if((vmg_ifo->vmgi_mat->vmgm_video_attr.display_aspect_ratio != 0) && (vmg_ifo->vmgi_mat->vmgm_video_attr.display_aspect_ratio != 3)) {
		printf("* VMG FAIL: Invalid aspect ratio: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.display_aspect_ratio);
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
	}

	// VMG Video: Invalid NTSC CC lines
	// An NTSC video can have lines set for field 1 and field 2.  Check
	// here if they are set while video is not NTSC
	if((vmg_ifo->vmgi_mat->vmgm_video_attr.video_format != 0) && (vmg_ifo->vmgi_mat->vmgm_video_attr.line21_cc_1 != 0)) {
		printf("* VMG FAIL: Line 21 CC 1 is set on non-NTSC video\n");
	}
	if((vmg_ifo->vmgi_mat->vmgm_video_attr.video_format != 0) && (vmg_ifo->vmgi_mat->vmgm_video_attr.line21_cc_2 != 0)) {
		printf("* VMG FAIL: Line 21 CC 2 is set on non-NTSC video\n");
	}

	// VMG Video: Invalid video resolution check
	// Valid values: 0, 1, 2, 3
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.picture_size > 3) {
		printf("* VMG FAIL: Invalid video format: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.picture_size);
	}

	// VMG Video: Invalid letterbox check
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.letterboxed > 1) {
		printf(" VMG FAIL: Invalid letterbox value: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.letterboxed);
	}

	// VMG Video: Invalid film type check
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->vmgm_video_attr.film_mode > 1) {
		printf("* VMG FAIL: Invalid film mode: %i\n", vmg_ifo->vmgi_mat->vmgm_video_attr.film_mode);
	}

	// VMG Audio: check number of audio streams
	// Valid values: 0, 1
	if(vmg_ifo->vmgi_mat->nr_of_vmgm_audio_streams > 1) {
		printf("* VMG FAIL: Invalid number of audio streams: %i\n", vmg_ifo->vmgi_mat->nr_of_vmgm_audio_streams);
	}

	// VMG Audio: check for valid audio format
	// Valid values: 0, 2, 3, 4, 6
	if(vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format == 1 ||
		vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format == 5 ||
		vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format > 6) {
		printf("* VMG FAIL: Invalid audio format number: %i\n", vmg_ifo->vmgi_mat->vmgm_audio_attr.audio_format);
	}

	// VMG Audio: check for valid quantization
	// Valid values: 0, 1, 2, 3
	if(vmg_ifo->vmgi_mat->vmgm_audio_attr.quantization > 3) {
		printf("* VMG FAIL: Invalid audio quantization: %i\n", vmg_ifo->vmgi_mat->vmgm_audio_attr.quantization);
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
	}

	// VMGM Subtitles: check for number of subp streams
	// Valid values: 0 (I think)
	if(vmg_ifo->vmgi_mat->vmgm_subp_attr.code_mode > 0) {
		printf("* VMG UNKNOWN: subpicture code mode is not zero: %i\n", vmg_ifo->vmgi_mat->vmgm_subp_attr.code_mode);
	}


	// Check for invalid IFOs
	for(vts = 1; vts < vmg_ifo->vts_atrt->nr_of_vtss + 1; vts++) {

		vts_ifo = ifoOpen(dvdread_dvd, vts);

		if(vts_ifo == NULL) {
			valid_ifos[vts] = false;
			printf("* IFO %u is invalid (possibly garbage)\n", vts);
		} else {
			valid_ifos[vts] = true;
			ifoClose(vts_ifo);
			vts_ifo = NULL;
		}

	}

	// Check for invalid IFOs

	for(title_track = 1; title_track <= title_tracks; title_track++) {

		length_mismatch = false;
		chapters_mismatch = false;

		vts = dvd_vts_ifo_number(vmg_ifo, title_track);

		if(!valid_ifos[vts])
			continue;

		vts_ifo = ifoOpen(dvdread_dvd, vts);
		vts_pgcit = vts_ifo->vts_pgcit;
		ttn = dvd_track_ttn(vmg_ifo, title_track);
		pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

		track_time = &pgc->playback_time;

		/*
		printf("dvd_time:	%02x:%02x:%02x.%02x	%u\n", track_time->hour, track_time->minute, track_time->second, track_time->frame_u & 0x3f, track_time->frame_u & 0xc0);
		printf("dvd_info:	%s\n", dvd_track_length(vmg_ifo, vts_ifo, title_track));
		*/

		title_track_msecs = dvd_track_msecs(vmg_ifo, vts_ifo, title_track);

		chapters = dvd_track_chapters(vmg_ifo, vts_ifo, title_track);
		nr_of_ptts = vmg_ifo->tt_srpt->title[title_track - 1].nr_of_ptts;

		if(nr_of_ptts != chapters)
			chapters_mismatch = true;

		chapters_msecs = 0;

		for(chapter = 1; chapter <= chapters; chapter++) {

			chapter_msecs = dvd_chapter_msecs(vmg_ifo, vts_ifo, title_track, chapter);
			chapters_msecs += chapter_msecs;

		}

		if(title_track_msecs != chapters_msecs)
			length_mismatch = true;

		cells = dvd_track_cells(vmg_ifo, vts_ifo, title_track);

		cells_msecs = 0;

		for(cell = 1; cell <= cells; cell++) {

			cell_msecs = dvd_cell_msecs(vmg_ifo, vts_ifo, title_track, cell);
			cells_msecs += cell_msecs;

		}

		if(title_track_msecs != cells_msecs)
			length_mismatch = true;

		if(length_mismatch) {

			printf("[Title Track %u Length Mismatch]\n", title_track);
			printf("* Title track:	%u\n", title_track_msecs);

			printf("* Chapters:	%u", chapters_msecs);
			if(title_track_msecs != chapters_msecs)
				printf("	%i offset\n", title_track_msecs - chapters_msecs);
			else
				printf("\n");

			printf("* Cells:	%u", cells_msecs);
			if(title_track_msecs != cells_msecs)
				printf("	%i offset\n", title_track_msecs - cells_msecs);
			else
				printf("\n");

		}

		if(chapters_mismatch) {

			printf("[Title Track %u Num. Chapters Mismatch]\n", title_track);
			printf("* VTS IFO PGC:	%u\n", chapters);
			printf("* VMG IFO PTTS:	%u\n", nr_of_ptts);

		}

		ifoClose(vts_ifo);
		vts_ifo = NULL;

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
	}
	// Title IFO Menu Video: check for invalid codec
	if(vts_ifo->vtsi_mat->vtsm_video_attr.mpeg_version > 1) {
		printf("* VTS Menu FAIL: Invalid video codec: %i\n", vts_ifo->vtsi_mat->vtsm_video_attr.mpeg_version);
	}
	// Title IFO Menu Video: Standard format (NTSC, PAL)
	if(vts_ifo->vtsi_mat->vtsm_video_attr.video_format > 1) {
		printf("* VTS Menu FAIL: Invalid video format: %i\n", vts_ifo->vtsi_mat->vtsm_video_attr.video_format);
	}
	// Title IFO Menu Video: Aspect Ratio
	// Valid values: 0 or 3 (4:3 and 16:9, respectively)
	if((vts_ifo->vtsi_mat->vtsm_video_attr.display_aspect_ratio != 0) && (vts_ifo->vtsi_mat->vtsm_video_attr.display_aspect_ratio != 3)) {
		printf("* VTS Menu FAIL: Invalid aspect ratio: %i\n", vts_ifo->vtsi_mat->vtsm_video_attr.display_aspect_ratio);
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
	}
	*/

	/**
	 * Track information
	 */

	/*
	for(track_number = 1; track_number <= num_search_pointers; track_number++) {

		printf("[Track %i]\n", track_number);

		vts_ifo = ifoOpen(dvdread_dvd, ifo_number);
		if(vts_ifo == NULL) {
			printf("Track %2d: opening IFO %d failed!\n", track_number, ifo_number);
			vts_ifo = NULL;
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

	return 0;

}
