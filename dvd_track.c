#include "dvd_track.h"
#include "dvd_audio.h"
#include "dvd_video.h"
#include "dvd_subtitles.h"

/**
 * Functions used to get information about a DVD track
 */

/**
 * Get the IFO number that a track resides in
 */
uint16_t dvd_vts_ifo_number(ifo_handle_t *vmg_ifo, uint16_t track_number) {

	// TODO research
	// Should these be the same number
	// vts_ttn = vmg_ifo->tt_srpt->title[title_track_idx].vts_ttn;

	uint16_t ifo_number = vmg_ifo->tt_srpt->title[track_number - 1].title_set_nr;

	return ifo_number;

}

uint8_t dvd_track_ttn(ifo_handle_t *vmg_ifo, uint16_t track_number) {

	uint8_t ttn = vmg_ifo->tt_srpt->title[track_number - 1].vts_ttn;

	return ttn;

}

uint16_t dvd_track_title_parts(ifo_handle_t *vmg_ifo, uint16_t track_number) {

	uint16_t nr_of_ptts = vmg_ifo->tt_srpt->title[track_number - 1].nr_of_ptts;

	return nr_of_ptts;

}

/**
 * Get the track's VTS id
 * Possible that it's blank, usually set to DVDVIDEO-VTS otherwise.
 */
bool dvd_vts_id(char *dest_str, ifo_handle_t *vts_ifo) {

	size_t i = 0;

	for(i = 0; i < strlen(vts_ifo->vtsi_mat->vts_identifier); i++) {
		if(!isalnum(vts_ifo->vtsi_mat->vts_identifier[i]))
			return false;
	}

	strncpy(dest_str, vts_ifo->vtsi_mat->vts_identifier, DVD_VTS_ID);

	return true;

}

uint8_t dvd_track_chapters(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number) {

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

uint8_t dvd_track_cells(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number) {

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

uint64_t dvd_track_blocks(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number) {

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

uint64_t dvd_track_filesize(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number) {

	uint64_t blocks;
	blocks = dvd_track_blocks(vmg_ifo, vts_ifo, track_number);

	uint64_t filesize = 0;
	filesize = blocks * DVD_VIDEO_LB_LEN;

	return filesize;

}

double dvd_track_filesize_mbs(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number) {

	uint64_t blocks;
	blocks = dvd_track_blocks(vmg_ifo, vts_ifo, track_number);

	if(blocks == 0)
		return 0;

	double track_filesize_mbs = 0;
	track_filesize_mbs = ceil((blocks * DVD_VIDEO_LB_LEN) / 1048576.0);

	return track_filesize_mbs;

}
