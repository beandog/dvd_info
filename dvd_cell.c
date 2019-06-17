#include "dvd_cell.h"

/**
 * Functions used to get information about a DVD cell
 */

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
