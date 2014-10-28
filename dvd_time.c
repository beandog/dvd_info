#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_time.h"

/**
 * For another reference implementation, see ifo_print_time() from libdvdread
 *
 * This function is confusing, and the code is floating around everywhere that
 * DVD playback is done in Linux to get the playback time.  I don't udnerstand
 * it because of the bit shifting, but it does work.
 *
 * The function is broken down a little bit more in  order to help me make it
 * easier to see what's going on.
 */
uint32_t dvd_time_to_milliseconds(dvd_time_t *dvd_time) {

	uint32_t framerates[4] = {0, 2500, 0, 2997};
	uint32_t framerate = 0;
	uint32_t hours = 0;
	uint32_t minutes = 0;
	uint32_t seconds = 0;
	uint32_t msecs = 0;
	uint32_t total = 0;

	framerate = framerates[(dvd_time->frame_u & 0xc0) >> 6];

	/*
	msecs = (((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f)) * 3600000;
	msecs += (((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f)) * 60000;
	msecs += (((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f)) * 1000;
	if(framerate > 0)
		msecs += (((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000 / framerate;
	*/
	hours = (((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f));
	total = (hours * 3600000);

	minutes = (((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f));
	total += (minutes * 60000);

	seconds = (((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f));
	total += (seconds * 1000);

	if(framerate > 0)
		msecs = ((((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000) / framerate;

	total += msecs;

	return total;

}

const char *milliseconds_length_format(const uint32_t milliseconds) {

	char chapter_length[12 + 1] = {'\0'};
	uint32_t total_seconds = 0;
	uint32_t hours = 0;
	uint32_t minutes = 0;
	uint32_t seconds = 0;
	uint32_t msecs = 0;

	total_seconds = milliseconds / 1000;
	hours = total_seconds / (3600);
	minutes = (total_seconds / 60) % 60;
	if(minutes > 59)
		minutes -= 59;
	seconds = total_seconds % 60;
	if(seconds > 59)
		seconds -= 59;
	msecs = milliseconds - (hours * 3600 * 1000) - (minutes * 60 * 1000) - (seconds * 1000);

	snprintf(chapter_length, 12 + 1, "%02u:%02u:%02u.%03u", hours, minutes, seconds, msecs);

	return strndup(chapter_length, 12);

}

uint32_t dvd_track_milliseconds(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t ttn;
	pgcit_t *vts_pgcit;
	pgc_t *pgc;
	uint32_t msecs;

	ttn = dvd_track_ttn(vmg_ifo, track_number);
	vts_pgcit = vts_ifo->vts_pgcit;
	pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc->cell_playback == NULL)
		return 0;

	msecs = dvd_time_to_milliseconds(&pgc->playback_time);

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
uint32_t dvd_chapter_milliseconds(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	uint8_t ttn;
	pgcit_t *vts_pgcit;
	pgc_t *pgc;
	uint32_t msecs;
	uint8_t chapters;
	uint8_t chapter_idx;
	uint8_t program_map_idx;
	uint8_t cell_idx;

	ttn = dvd_track_ttn(vmg_ifo, track_number);
	vts_pgcit = vts_ifo->vts_pgcit;
	pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;
	msecs = 0;
	chapters = 0;
	chapter_idx = 0;
	program_map_idx = 0;
	cell_idx = 0;

	if(pgc->cell_playback == NULL || pgc->program_map == NULL)
		return 0;

	chapters = pgc->nr_of_programs;

	// FIXME it'd be better, and more helpful as a reference, to simply get the
	// cells that are in a chapter, and then add up the lengths of those cells.
	// So, something similar to chapter_startcell, chapter_stopcell as well.
	for(chapter_idx = 0; chapter_idx < pgc->nr_of_programs; chapter_idx++) {

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

uint32_t dvd_cell_milliseconds(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number) {

	uint8_t ttn;
	pgcit_t *vts_pgcit;
	pgc_t *pgc;
	uint32_t msecs;

	ttn = dvd_track_ttn(vmg_ifo, track_number);
	vts_pgcit = vts_ifo->vts_pgcit;
	pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;
	msecs = 0;

	if(pgc->cell_playback == NULL)
		return 0;

	msecs = dvd_time_to_milliseconds(&pgc->cell_playback[cell_number - 1].playback_time);

	return msecs;

}

const char *dvd_track_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint32_t msecs = 0;

	msecs = dvd_track_milliseconds(vmg_ifo, vts_ifo, track_number);

	return strndup(milliseconds_length_format(msecs), DVD_TRACK_LENGTH);

}

const char *dvd_chapter_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	uint32_t msecs;

	msecs = dvd_chapter_milliseconds(vmg_ifo, vts_ifo, track_number, chapter_number);

	return strndup(milliseconds_length_format(msecs), DVD_CHAPTER_LENGTH);

}


const char *dvd_cell_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number) {

	uint32_t msecs;

	msecs = dvd_cell_milliseconds(vmg_ifo, vts_ifo, track_number, cell_number);

	return strndup(milliseconds_length_format(msecs), DVD_CELL_LENGTH);

}
