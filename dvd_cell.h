#ifndef DVD_INFO_CELL_H
#define DVD_INFO_CELL_H

#include "dvd_track.h"
#include "dvd_vmg_ifo.h"

struct dvd_cell {
	uint8_t cell;
	char length[DVD_CELL_LENGTH + 1];
	uint32_t msecs;
	uint32_t first_sector;
	uint32_t last_sector;
	ssize_t blocks;
	ssize_t filesize;
};

uint32_t dvd_cell_first_sector(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

uint32_t dvd_cell_last_sector(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

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
ssize_t dvd_cell_blocks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

ssize_t dvd_cell_filesize(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

#endif
