#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_time.h"

/**
 * Convert any value of dvd_time to milliseconds
 * For another reference implementation, see ifo_print_time() from libdvdread
 *
 * This function is confusing, and the code is floating around everywhere that
 * DVD playback is done in Linux to get the playback time.  I don't udnerstand
 * it because of the bit shifting, but it does work.
 *
 * The function is broken down a little bit more in  order to help me make it
 * easier to see what's going on.
 *
 * @param dvd_time dvd_time
 */
uint32_t dvd_time_to_milliseconds(dvd_time_t *dvd_time) {

	uint32_t msecs = 0;
	uint32_t framerates[4] = {0, 2500, 0, 2997};
	uint32_t framerate = framerates[(dvd_time->frame_u & 0xc0) >> 6];

	/*
	msecs = (((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f)) * 3600000;
	msecs += (((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f)) * 60000;
	msecs += (((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f)) * 1000;
	if(framerate > 0)
		msecs += (((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000 / framerate;
	*/

	uint32_t hours = (((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f));
	uint32_t minutes = (((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f));
	uint32_t seconds = (((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f));
	if(framerate > 0)
		msecs = ((((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000) / framerate;

	uint32_t total = (hours * 3600000);
	total += (minutes * 60000);
	total += (seconds * 1000);
	total += msecs;

	return total;

}

/**
 * Convert milliseconds to format hh:mm:ss.ms
 *
 * @param milliseconds milliseconds
 */
const char *milliseconds_length_format(const uint32_t milliseconds) {

	char chapter_length[12 + 1] = {'\0'};

	uint32_t total_seconds = milliseconds / 1000;
	uint32_t hours = total_seconds / (3600);
	uint32_t minutes = (total_seconds / 60) % 60;
	if(minutes > 59)
		minutes -= 59;
	uint32_t seconds = total_seconds % 60;
	if(seconds > 59)
		seconds -= 59;
	uint32_t msecs = milliseconds - (hours * 3600 * 1000) - (minutes * 60 * 1000) - (seconds * 1000);

	snprintf(chapter_length, 12 + 1, "%02u:%02u:%02u.%03u", hours, minutes, seconds, msecs);

	return strndup(chapter_length, 12);

}

/**
 * Get the number of milliseconds for a title track
 *
 * There are cases where the time for the track and the total of the cells do
 * not match up, generally in a range of -5 to 5 *milliseconds*.  Instead of
 * looking at the PGC for the title track, instead use the cell as the base
 * reference and get the total from those.
 */
uint32_t dvd_track_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

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
	// So, something similar to chapter_startcell, chapter_stopcell as well.
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
 * Get the number of milliseconds of a cell
 */
uint32_t dvd_cell_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc->cell_playback == NULL)
		return 0;

	uint32_t msecs = dvd_time_to_milliseconds(&pgc->cell_playback[cell_number - 1].playback_time);

	return msecs;

}

/**
 * Get the formatted string length of a title track
 */
const char *dvd_track_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint32_t msecs = dvd_track_msecs(vmg_ifo, vts_ifo, track_number);

	return strndup(milliseconds_length_format(msecs), DVD_TRACK_LENGTH);

}

/**
 * Get the formatted string length of a chapter
 */
const char *dvd_chapter_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	uint32_t msecs = dvd_chapter_msecs(vmg_ifo, vts_ifo, track_number, chapter_number);

	return strndup(milliseconds_length_format(msecs), DVD_CHAPTER_LENGTH);

}

/**
 * Get the formatted string length of a cell
 */
const char *dvd_cell_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number) {

	uint32_t msecs = dvd_cell_msecs(vmg_ifo, vts_ifo, track_number, cell_number);

	return strndup(milliseconds_length_format(msecs), DVD_CELL_LENGTH);

}
