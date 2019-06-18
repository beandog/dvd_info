#ifndef DVD_INFO_CELL_H
#define DVD_INFO_CELL_H

#include "dvd_track.h"
#include "dvd_vmg_ifo.h"

struct dvd_cell {
	uint8_t cell;
	char length[DVD_CELL_LENGTH + 1];
	uint32_t msecs;
	uint64_t first_sector;
	uint64_t last_sector;
	uint64_t blocks;
	uint64_t filesize;
	double filesize_mbs;
};

uint64_t dvd_cell_first_sector(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

uint64_t dvd_cell_last_sector(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

/**
 * Helper function to get the total number of blocks from a cell.
 *
 * Understanding the difference between cell sectors and cell blocks can be a bit confusing,
 * so this is here to make things simpler.
 *
 * One cell sector is the same as one cell block. Since the dvdread function DVDReadBlocks
 * asks for a number of blocks to read (not sectors), this matches the same
 * naming.
 *
 */
uint64_t dvd_cell_blocks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

uint64_t dvd_cell_filesize(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

double dvd_cell_filesize_mbs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

/**
 * Track some possible indicators that the DVD is authored intentionally to break
 * software using libdvdread.
 *
 * Cells are the best indicator that something is suspicious. Since they are the points on
 * the filesystem where the data begins and ends, there are some expectations on the
 * order and layout of how they should be, and if they break those rules, it's likely
 * that the DVD was authored with the intention to break playback / access / ripping, etc.
 *
 * DVDs generally (more research needed) have all their sectors in sequential numbers.
 * Meaning that one cell could be sector 4092 to 7136, the next cell being 7137 to 8198,
 * then 8199 to 100842, etc. If they are not ordered that way, there's two likely
 * scenarios -- one is the program chain telling the player to navigate back to something,
 * and these are ordered at the *end* of a track, or the cell sector locations are
 * intentionally bouncing around to break things trying to access the filesystem.
 *
 * An example of a sequential structure:
 * Cell: 01, Length: 00:02:24.567 First sector: 4112, Last sector: 53741
 * Cell: 02, Length: 00:05:20.100 First sector: 53742, Last sector: 103173
 * Cell: 03, Length: 00:06:42.700 First sector: 103174, Last sector: 148755
 * Cell: 04, Length: 00:08:24.300 First sector: 148756, Last sector: 197145
 * Cell: 05, Length: 00:00:46.500 First sector: 197146, Last sector: 243741
 *
 * An example of an unusual structure:
 * Cell: 01, Length: 00:02:24.567 First sector: 4112, Last sector: 59170
 * Cell: 02, Length: 00:00:46.333 First sector: 993631, Last sector: 1001433
 * Cell: 03, Length: 00:00:46.266 First sector: 2473989, Last sector: 2481483
 * Cell: 04, Length: 00:03:11.533 First sector: 1489720, Last sector: 1560941
 * Cell: 05, Length: 00:06:22.533 First sector: 737502, Last sector: 877967
 *
 * These functions do the checking to see what issues are present, and flag the DVD
 * as having those errors. For a program that backs up the DVD, it's relevant so that
 * it knows which tracks to skip, so that your 6 GB DVD doesn't back up to 60 GB.
 *
 */

/*
 * Check the sectors order in two ways. First, see if the minimum sector number ever
 * goes down over the first one. Second, if the maximum sector on any cell is
 * lower than the highest one during the loop.
 */
bool dvd_track_min_sector_error(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);
bool dvd_track_max_sector_error(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

/**
 * Check to see if the first or last sector is used more than once.
 */
bool dvd_track_repeat_first_sector_error(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

bool dvd_track_repeat_last_sector_error(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

#endif
