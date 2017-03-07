#include "dvd_track.h"
#include "dvd_cell.h"

/**
 * Functions used to get information about a DVD cell
 */


uint32_t dvd_cell_first_sector(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc == NULL || pgc->program_map == NULL)
		return 0;
	
	uint32_t sector = 0;
	sector = pgc->cell_playback[cell_number - 1].first_sector;

	return sector;

}

uint32_t dvd_cell_last_sector(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc == NULL || pgc->program_map == NULL)
		return 0;
	
	uint32_t sector = 0;
	sector = pgc->cell_playback[cell_number - 1].last_sector;

	return sector;

}

uint32_t dvd_cell_blocks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number) {

	uint32_t first_sector = 0;
	first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, track_number, cell_number);

	uint32_t last_sector = 0;
	last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, track_number, cell_number);

	uint32_t blocks = 0;
	blocks = last_sector - first_sector;

	// Include the last cell
	if(last_sector >= first_sector)
		blocks++;

	return blocks;

}
