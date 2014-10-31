#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_time.h"

/**
 * Convert any value of dvd_time to milliseconds
 *
 * In other DVD applications, 29.97 FPS is used in place of 30.  I've changed
 * mine for a couple of reasons.
 *
 * Originally, the title track length was calculated by looking at the PGC for
 * the track itself.  With that approach, the total length for the track did
 * not match the sum of the total length of all the individual cells.  The
 * offset was small, in the range of -5 to +5 *milliseconds* and it did not
 * occur all the time.  To fix the title track length, I changed it to use the
 * total from all the cells.  That by itself changed the track lengths to msec
 * offsets that looked a little strange from their original parts.  Fex, 500
 * would change to 501, 900 to 898, etc.  Since it's far more likely that the
 * correct value was a whole integer (.500 seconds happens all the time), then
 * it needed to be changed.
 *
 * According to http://stnsoft.com/DVD/pgc.html it looks like the FPS values are
 * integers, either 25 or 30.  So this uses 3000 instead of 2997 for that reason.
 * as well.  With these two changes only does minor changes, and they are always
 * something like 734 to 733, 667 to 666, 834 to 833, and so on.
 *
 * Either way, it's probably safe to say that calculating the exact length of a
 * cell or track is hard, and that this is the best approxmiation that
 * preserves what the original track length is estimated to be from the PGC. So
 * this method is used as a preference, even though I don't necessarily want to
 * recommend relying on either approach for complete accuracy.
 *
 * Making these changes though is going to differ in displaying the track
 * length from other DVD applications -- again, in milliseconds only -- but I
 * justify this approach in the sense that using this way, the cell, chapter
 * and title track lengths all match up.
 *
 * Regarding the bit-shifting in the function, I've broken down the
 * calculations to separate individual time metrics to make it a bit easier to
 * read, though I admittedly don't know enough about bit shifting to know what
 * it's doing or why it doesn't just do simple calculation.  It's worth seeing
 * ifo_print_time() from libdvdread as a comparative reference as well.
 *
 * @param dvd_time dvd_time
 */
uint32_t dvd_time_to_milliseconds(dvd_time_t *dvd_time) {

	uint32_t msecs = 0;
	uint32_t framerates[4] = {0, 2500, 0, 3000};
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
