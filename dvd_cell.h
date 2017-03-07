#ifndef _DVD_CELL_H_
#define _DVD_CELL_H_

#include "dvd_info.h"

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
uint32_t dvd_cell_blocks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

ssize_t dvd_cell_filesize(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t cell_number);

#endif
