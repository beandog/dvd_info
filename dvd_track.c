#include "dvd_track.h"
#include "dvd_video.h"
#include "dvd_time.h"

/**
 * Functions used to get information about a DVD track
 */

/**
 * Get the IFO number that a track resides in
 *
 * @param vmg_ifo dvdread handler for primary IFO
 * @return IFO number
 */
uint16_t dvd_vts_ifo_number(const ifo_handle_t *vmg_ifo, const uint16_t track_number) {

	// TODO research
	// Should these be the same number
	// vts_ttn = vmg_ifo->tt_srpt->title[title_track_idx].vts_ttn;

	uint16_t ifo_number = vmg_ifo->tt_srpt->title[track_number - 1].title_set_nr;

	return ifo_number;

}

uint64_t dvd_cell_first_sector(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc == NULL || pgc->program_map == NULL)
		return 0;

	uint64_t sector = 0;
	sector = (uint64_t)pgc->cell_playback[cell_number - 1].first_sector;

	return sector;

}

uint64_t dvd_cell_last_sector(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc == NULL || pgc->program_map == NULL)
		return 0;

	uint64_t sector = 0;
	sector = (uint64_t)pgc->cell_playback[cell_number - 1].last_sector;

	return sector;

}

uint64_t dvd_cell_blocks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number) {

	uint64_t first_sector = 0;
	first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, track_number, cell_number);

	uint64_t last_sector = 0;
	last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, track_number, cell_number);

	uint64_t blocks = 0;
	blocks = last_sector - first_sector;

	// Include the last cell
	if(last_sector == first_sector)
		blocks++;

	return blocks;

}

uint64_t dvd_cell_filesize(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number) {

	uint64_t blocks;
	blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, track_number, cell_number);

	uint64_t filesize = 0;
	filesize = blocks * DVD_VIDEO_LB_LEN;

	return filesize;

}

double dvd_cell_filesize_mbs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number) {

	uint64_t blocks;
	blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, track_number, cell_number);

	double cell_filesize_mbs = 0;
	cell_filesize_mbs = ceil((blocks * DVD_VIDEO_LB_LEN) / 1048576.0);

	return cell_filesize_mbs;

}

bool dvd_track_min_sector_error(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t cells = dvd_track_cells(vmg_ifo, vts_ifo, track_number);

	if(!cells || cells == 1)
		return false;

	uint8_t cell = 2;
	uint64_t first_sector = 0;
	uint64_t min_sector = 0;

	min_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, track_number, 1);

	for(cell = 2; cell < cells + 1; cell++) {

		first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, track_number, cell);

		if(first_sector < min_sector)
			return true;

		min_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, track_number, cell);

	}

	return false;

}

bool dvd_track_max_sector_error(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t cells = dvd_track_cells(vmg_ifo, vts_ifo, track_number);

	if(!cells || cells == 1)
		return false;

	uint8_t cell = 2;
	uint64_t last_sector = 0;
	uint64_t max_sector = 0;

	max_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, track_number, 1);

	for(cell = 2; cell < cells + 1; cell++) {

		last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, track_number, cell);

		if(last_sector < max_sector)
			return true;

		max_sector = last_sector;

	}

	return false;

}

bool dvd_track_repeat_first_sector_error(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t cells = dvd_track_cells(vmg_ifo, vts_ifo, track_number);

	if(!cells || cells == 1)
		return false;

	uint8_t cell = 2;
	uint64_t first_cell_first_sector = 0;
	uint64_t first_sector = 0;

	first_cell_first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, track_number, 1);

	for(cell = 2; cell < cells + 1; cell++) {

		first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, track_number, cell);

		if(first_sector == first_cell_first_sector)
			return true;

	}

	return false;

}

bool dvd_track_repeat_last_sector_error(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t cells = dvd_track_cells(vmg_ifo, vts_ifo, track_number);

	if(!cells || cells == 1)
		return false;

	uint8_t cell = 2;
	uint64_t first_cell_last_sector = 0;
	uint64_t last_sector = 0;

	first_cell_last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, track_number, 1);

	for(cell = 2; cell < cells + 1; cell++) {

		last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, track_number, cell);

		if(last_sector == first_cell_last_sector)
			return true;

	}

	return false;

}

/**
 * Get the number of milliseconds of a cell
 */
uint32_t dvd_cell_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number) {

	if(vts_ifo->vts_pgcit == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc->cell_playback == NULL)
		return 0;

	uint32_t msecs = dvd_time_to_milliseconds(&pgc->cell_playback[cell_number - 1].playback_time);

	return msecs;

}

/**
 * Get the formatted string length of a cell
 */
void dvd_cell_length(char *dest_str, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number) {

	uint32_t msecs = dvd_cell_msecs(vmg_ifo, vts_ifo, track_number, cell_number);

	milliseconds_length_format(dest_str, msecs);

}

dvd_cell_t *dvd_cell_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t cell_number) {

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo) || vts_ifo == NULL)
		return NULL;

	dvd_cell_t *dvd_cell = calloc(1, sizeof(dvd_cell_t));

	if(dvd_cell == NULL)
		return NULL;

	dvd_cell->cell = cell_number;
	dvd_cell->first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, track_number, cell_number);
	dvd_cell->last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, track_number, cell_number);
	dvd_cell->blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, track_number, cell_number);
	dvd_cell->filesize = dvd_cell_filesize(vmg_ifo, vts_ifo, track_number, cell_number);
	dvd_cell->filesize_mbs = dvd_cell_filesize_mbs(vmg_ifo, vts_ifo, track_number, cell_number);
	dvd_cell->msecs = dvd_cell_msecs(vmg_ifo, vts_ifo, track_number, cell_number);
	dvd_cell_length(dvd_cell->length, vmg_ifo, vts_ifo, track_number, cell_number);

	return dvd_cell;

}

uint8_t dvd_track_ttn(const ifo_handle_t *vmg_ifo, const uint16_t track_number) {

	uint8_t ttn = vmg_ifo->tt_srpt->title[track_number - 1].vts_ttn;

	return ttn;

}

uint8_t dvd_track_chapters(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	// If there's no cell playback, then override the number in the PGC and report as
	// zero so that they are not accessed.
	if(pgc == NULL || pgc->cell_playback == NULL)
		return 0;

	uint8_t chapters = pgc->nr_of_programs;

	return chapters;

}

uint8_t dvd_track_cells(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);

	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	if(vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc == NULL)
		return 0;

	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	// If there's no cell playback, then override the number in the PGC and report as
	// zero so that they are not accessed.
	if(pgc == NULL || pgc->cell_playback == NULL)
		return 0;

	uint8_t cells = pgc->nr_of_cells;

	return cells;

}

uint64_t dvd_track_blocks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t cells;
	cells = dvd_track_cells(vmg_ifo, vts_ifo, track_number);

	uint8_t cell;
	uint64_t cell_blocks;
	uint64_t track_blocks;
	track_blocks = 0;
	for(cell = 1; cell < cells + 1; cell++) {

		cell_blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, track_number, cell);

		track_blocks += cell_blocks;

	}

	return track_blocks;

}

uint64_t dvd_track_filesize(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint64_t blocks;
	blocks = dvd_track_blocks(vmg_ifo, vts_ifo, track_number);

	uint64_t filesize = 0;
	filesize = blocks * DVD_VIDEO_LB_LEN;

	return filesize;

}

double dvd_track_filesize_mbs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint64_t blocks;
	blocks = dvd_track_blocks(vmg_ifo, vts_ifo, track_number);

	if(blocks == 0)
		return 0;

	double track_filesize_mbs = 0;
	track_filesize_mbs = ceil((blocks * DVD_VIDEO_LB_LEN) / 1048576.0);

	return track_filesize_mbs;

}

/**
 * Get the number of audio streams for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @return number of audio streams
 */
uint8_t dvd_track_audio_tracks(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat == NULL)
		return 0;

	uint8_t audio_streams = vts_ifo->vtsi_mat->nr_of_vts_audio_streams;

	return audio_streams;

}

/**
 * Examine the PGC for the track IFO directly and see if there are any audio
 * control entries marked as active.  This is an alternative way of checking
 * for the number of audio streams, compared to looking at the VTS directly.
 * This is useful for debugging, and flushing out either badly mastered DVDs or
 * getting a closer identifier of how many streams this has.
 *
 * Some software uses this number of audio streams in the pgc instead of the
 * one in the VTSI MAT, such as mplayer and HandBrake, which will skip over
 * the other ones completely.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @return number of PGC audio streams marked as active
 */
uint8_t dvd_audio_active_tracks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track) {

	if(title_track == 0)
		return 0;

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;
	uint8_t idx = 0;
	uint8_t audio_tracks = 0;

	if(!pgc)
		return 0;

	for(idx = 0; idx < DVD_AUDIO_STREAM_LIMIT; idx++) {

		if(pgc->audio_control[idx] & 0x8000)
			audio_tracks++;

	}

	return audio_tracks;

}

/**
 * Get the number of subtitle streams for a track
 *
 */
uint8_t dvd_track_subtitles(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat == NULL)
		return 0;

	uint8_t subtitles = vts_ifo->vtsi_mat->nr_of_vts_subp_streams;

	return subtitles;

}

/**
 * Get the number of subtitle streams marked as active.
 *
 */
uint8_t dvd_track_active_subtitles(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;
	uint8_t idx = 0;
	uint8_t active_subtitles = 0;

	if(!pgc)
		return 0;

	for(idx = 0; idx < DVD_SUBTITLE_STREAM_LIMIT; idx++) {

		if(pgc->subp_control[idx] & 0x80000000)
			active_subtitles++;

	}

	return active_subtitles;

}

/**
 * Get the number of milliseconds for a title track using the program chain.
 *
 */
uint32_t dvd_track_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	if(vts_ifo->vts_pgcit == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc == NULL || pgc->cell_playback == NULL)
		return 0;

	uint32_t msecs = dvd_time_to_milliseconds(&pgc->playback_time);

	return msecs;

}

/**
 * Get the formatted string length of a title track
 */
void dvd_track_length(char *dest_str, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint32_t msecs = dvd_track_msecs(vmg_ifo, vts_ifo, track_number);

	milliseconds_length_format(dest_str, msecs);

}

dvd_track_t *dvd_track_init(dvd_reader_t *dvdread_dvd, ifo_handle_t *vmg_ifo, uint16_t track) {

	dvd_track_t *dvd_track = calloc(1, sizeof(dvd_track_t));

	if(!dvd_track)
		return NULL;

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo))
		return NULL;

	uint16_t vts = dvd_vts_ifo_number(vmg_ifo, track);
	ifo_handle_t *vts_ifo = ifoOpen(dvdread_dvd, vts);

	if(vts_ifo == NULL)
		return NULL;

	dvd_track->track = track;
	dvd_track->vts = vts;

	dvd_track->valid = true;
	dvd_track->vts = dvd_vts_ifo_number(vmg_ifo, track);

	/** Length **/
	memset(dvd_track->length, '\0', sizeof(dvd_track->length));
	dvd_track_length(dvd_track->length, vmg_ifo, vts_ifo, track);
	dvd_track->msecs = dvd_track_msecs(vmg_ifo, vts_ifo, track);

	/** Structure **/
	dvd_track->chapters = dvd_track_chapters(vmg_ifo, vts_ifo, track);
	dvd_track->cells = dvd_track_cells(vmg_ifo, vts_ifo, track);

	/** Filesize **/
	dvd_track->blocks = dvd_track_blocks(vmg_ifo, vts_ifo, track);
	dvd_track->filesize = dvd_track->blocks * DVD_VIDEO_LB_LEN;
	dvd_track->filesize_mbs = 0;
	if(dvd_track->filesize > 0)
		dvd_track->filesize_mbs = ceil((dvd_track->blocks * DVD_VIDEO_LB_LEN) / 1048576.0);

	/** Audio **/
	dvd_track->audio_tracks = dvd_track_audio_tracks(vts_ifo);
	dvd_track->active_audio_streams = dvd_audio_active_tracks(vmg_ifo, vts_ifo, track);

	/** Subtitles **/
	dvd_track->subtitles = dvd_track_subtitles(vts_ifo);
	dvd_track->active_subs = dvd_track_active_subtitles(vmg_ifo, vts_ifo, track);

	return dvd_track;

}
