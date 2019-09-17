#include "dvd_vmg_ifo.h"
#include "dvd_track.h"
#include "dvd_chapter.h"
#include "dvd_time.h"

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

/**
 * OLD NOTES, not relevant any more since it calculates all cell lengths:
 * Sourced from lsdvd.c.  I don't understand the logic behind it, and why the
 * original doesn't access pgc->cell_playback[cell_idx].playback_time directly.
 * Two things I do know: not to assume a cell is a chapter, and lsdvd's chapter
 * output has always worked for me, so I'm leaving it alone for now.
 *
 * This loops through *all* the chapters and gets the times, but only quits
 * once the specified one has been found.
 * END OLD NOTES
 */
/**
 * Get the number of milliseconds of a chapter
 */
uint32_t dvd_chapter_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	if(vts_ifo->vts_pgcit == NULL)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc->cell_playback == NULL || pgc->program_map == NULL)
		return 0;

	uint8_t chapters = pgc->nr_of_programs;
	uint8_t chapter_idx = 0;
	uint8_t program_map_idx = 0;
	uint8_t cell_idx = 0;
	uint32_t msecs = 0;

	// FIXME it'd be better, and more helpful as a reference, to simply get the
	// cells that are in a chapter, and then add up the lengths of those cells.
	// So, something similar to chapter_first_cell, chapter_stopcell as well.
	for(chapter_idx = 0; chapter_idx < chapters; chapter_idx++) {

		program_map_idx = pgc->program_map[chapter_idx + 1];

		if(chapter_idx == chapters - 1)
			program_map_idx = pgc->nr_of_cells + 1;

		while(cell_idx < program_map_idx - 1) {
			if(chapter_idx + 1 == chapter_number) {
				msecs += dvd_time_to_milliseconds(&pgc->cell_playback[cell_idx].playback_time);
			}
			cell_idx++;
		}

	}

	return msecs;

}

/**
 * Get the number of milliseconds for a title track by totalling the value of
 * all chapter length.
 *
 * This function is to be used for debugging or development.  The total msecs
 * of a title track should obviously be the same total of the length of all
 * the chapters (and cells) that add up, but this is not the case.  It is
 * often off by a few milliseconds (-5 to +5), but in some rare cases it can
 * be a total of minutes.
 *
 * The original dvd_track_msecs() function looks at the program chain for the
 * total, and that method is used across the board for pretty much every DVD
 * application that uses libdvdread.  So, this is part of dvd_info only, and
 * is used to flag anomalies using the dvd_debug program.
 *
 */
uint32_t dvd_track_total_chapter_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t chapters = dvd_track_chapters(vmg_ifo, vts_ifo, track_number);

	if(chapters == 0)
		return 0;

	uint32_t msecs = 0;
	uint8_t chapter;

	for(chapter = 1; chapter <= chapters; chapter++)
		msecs += dvd_chapter_msecs(vmg_ifo, vts_ifo, track_number, chapter);

	return msecs;

}

/**
 * Get the formatted string length of a chapter
 */
void dvd_chapter_length(char *dest_str, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	uint32_t msecs = dvd_chapter_msecs(vmg_ifo, vts_ifo, track_number, chapter_number);

	milliseconds_length_format(dest_str, msecs);

}

dvd_chapter_t *dvd_chapter_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number) {

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo) || vts_ifo == NULL)
		return NULL;

	dvd_chapter_t *dvd_chapter = calloc(1, sizeof(dvd_chapter_t));

	if(dvd_chapter == NULL)
		return NULL;

	dvd_chapter->chapter = chapter_number;
	dvd_chapter->msecs = dvd_chapter_msecs(vmg_ifo, vts_ifo, track_number, chapter_number);
	memset(dvd_chapter->length, '\0', sizeof(dvd_chapter->length));
	dvd_chapter_length(dvd_chapter->length, vmg_ifo, vts_ifo, track_number, chapter_number);
	dvd_chapter->first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, track_number, chapter_number);
	dvd_chapter->last_cell = dvd_chapter_last_cell(vmg_ifo, vts_ifo, track_number, chapter_number);

	return dvd_chapter;

}
