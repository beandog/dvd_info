#include "dvd_track.h"
#include "dvd_chapter.h"

// It's a *safe guess* that if the program_map is NULL or there is no starting cell,
// that the starting cell is actually the same as the chapter number.  See DVD id 79,
// track #12 for an example where this matches (NULL values)
//
// TODO some closer examination to check that cells and chapters match up properly
// is probably in order.  Some scenarios to look for would be where there are more
// chapters than cells, and the lengths of chapters don't exceed cells either.
uint8_t dvd_chapter_first_cell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc == NULL || pgc->program_map == NULL)
		return chapter_number;

	uint8_t first_cell = pgc->program_map[chapter_number - 1];

	if(first_cell > 0)
		return first_cell;
	else
		return chapter_number;

}
