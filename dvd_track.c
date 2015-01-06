#include "dvd_track.h"

/**
 * Functions used to get information about a DVD track
 */

/**
 * Check if the IFO is a VTS or not.
 *
 * libdvdread populates the ifo_handle with various data, but the structure is
 * the same for both a VMG IFO and a VTS one.  This does a few checks to make
 * sure that the ifo_handle passed in is a Video Title Set.
 *
 * @param ifo dvdread IFO handle
 * @return boolean
 */
bool ifo_is_vts(const ifo_handle_t *ifo) {

	if(ifo->vtsi_mat == NULL)
		return false;
	else
		return true;

}

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

	if(ifo_number >= 0)
		return ifo_number;
	else
		return 1;

}

uint8_t dvd_track_ttn(const ifo_handle_t *vmg_ifo, const uint16_t track_number) {

	uint8_t ttn = vmg_ifo->tt_srpt->title[track_number - 1].vts_ttn;

	if(ttn > 0)
		return ttn;
	else
		return 1;

}

/**
 * Get the track's VTS id
 * Possible that it's blank, usually set to DVDVIDEO-VTS otherwise.
 *
 * @param vts_ifo libdvdread IFO handle
 */
const char *dvd_vts_id(const ifo_handle_t *vts_ifo) {

	size_t i = 0;

	for(i = 0; i < strlen(vts_ifo->vtsi_mat->vts_identifier); i++) {
		if(!isascii(vts_ifo->vtsi_mat->vts_identifier[i]))
			return "";
	}

	return strndup(vts_ifo->vtsi_mat->vts_identifier, DVD_VTS_ID);

}

uint8_t dvd_track_chapters(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	// If there's no cell playback, then override the number in the PGC and report as
	// zero so that they are not accessed.
	if(pgc == NULL || pgc->cell_playback == NULL)
		return 0;

	uint8_t chapters = pgc->nr_of_programs;

	if(chapters > 0)
		return chapters;
	else
		return 0;

}

uint8_t dvd_track_cells(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	// If there's no cell playback, then override the number in the PGC and report as
	// zero so that they are not accessed.
	if(pgc == NULL || pgc->cell_playback == NULL)
		return 0;

	uint8_t cells = pgc->nr_of_cells;

	if(cells >= 0)
		return cells;
	else
		return 0;

}

// It's a *safe guess* that if the program_map is NULL or there is no starting cell,
// that the starting cell is actually the same as the chapter number.  See DVD id 79,
// track #12 for an example where this matches (NULL values)
//
// TODO some closer examination to check that cells and chapters match up properly
// is probably in order.  Some scenarios to look for would be where there are more
// chapters than cells, and the lengths of chapters don't exceed cells either.
uint8_t dvd_chapter_startcell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc == NULL || pgc->program_map == NULL)
		return chapter_number;

	uint8_t startcell = pgc->program_map[chapter_number - 1];

	if(startcell > 0)
		return startcell;
	else
		return chapter_number;

}
