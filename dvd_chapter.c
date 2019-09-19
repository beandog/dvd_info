#include "dvd_vmg_ifo.h"
#include "dvd_track.h"
#include "dvd_chapter.h"

/** It's a *safe guess* that if the program_map is NULL or there is no starting cell,
 * that the starting cell is actually the same as the chapter number.  See DVD id 79,
 * track #12 for an example where this matches (NULL values)
 *
 * TODO some closer examination to check that cells and chapters match up properly
 * is probably in order.  Some scenarios to look for would be where there are more
 * chapters than cells, and the lengths of chapters don't exceed cells either. Doing
 * so would be a good fit for dvd_debug program.
 */
uint8_t dvd_chapter_first_cell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return chapter_number;

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

/**
 * We know how many cells are on the track, and I'm somewhat guessing where the
 * first cell is on a chapter, so I'm not totally confident that this is always
 * correct -- but I'm saying that with the basis that there could be some
 * oddly mastered DVDs out there or some tricky navigation instructions. In
 * short, I'm overly cautious, and this will probably work just fine.
 */
uint8_t dvd_chapter_last_cell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return chapter_number;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc == NULL || pgc->program_map == NULL)
		return chapter_number;

	uint8_t track_chapters = dvd_track_chapters(vmg_ifo, vts_ifo, track_number);

	if(chapter_number == track_chapters)
		return pgc->nr_of_cells;

	uint8_t first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, track_number, chapter_number);

	uint8_t last_cell = first_cell;

	if(chapter_number < track_chapters) {

		uint8_t next_chapter_first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, track_number, chapter_number + 1);

		last_cell = next_chapter_first_cell - 1;

	}

	return last_cell;

}

uint8_t dvd_chapter_cells(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number) {

	uint8_t first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, track_number, chapter_number);
	uint8_t last_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, track_number, chapter_number);

	return last_cell - first_cell + 1;

}

uint64_t dvd_chapter_blocks(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number) {

	uint8_t first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, track_number, chapter_number);
	uint8_t last_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, track_number, chapter_number);

	uint8_t cell;
	uint64_t cell_blocks = 0;
	uint64_t chapter_blocks = 0;

	for(cell = first_cell; cell < last_cell + 1; cell++) {

		cell_blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, track_number, cell);

		chapter_blocks += cell_blocks;

	}

	return chapter_blocks;

}

uint64_t dvd_chapter_filesize(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number) {

	uint64_t blocks;
	blocks = dvd_chapter_blocks(vmg_ifo, vts_ifo, track_number, chapter_number);

	uint64_t filesize = 0;
	filesize = blocks * DVD_VIDEO_LB_LEN;

	return filesize;

}

double dvd_chapter_filesize_mbs(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number) {

	uint64_t blocks;
	blocks = dvd_chapter_blocks(vmg_ifo, vts_ifo, track_number, chapter_number);

	if(blocks == 0)
		return 0;

	double filesize_mbs = 0;
	filesize_mbs = (blocks * DVD_VIDEO_LB_LEN) / 1048576.0;

	if(filesize_mbs < 1.0)
		filesize_mbs = 1.0;

	return filesize_mbs;

}
