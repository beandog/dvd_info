#ifndef DVD_INFO_CHAPTER_H
#define DVD_INFO_CHAPTER_H

#include "dvd_cell.h"

struct dvd_chapter {
	uint8_t chapter;
	char length[DVD_CHAPTER_LENGTH + 1];
	uint32_t msecs;
	uint8_t first_cell;
	uint8_t last_cell;
	uint64_t blocks;
	uint64_t filesize;
	double filesize_mbs;
};

uint8_t dvd_chapter_first_cell(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

uint8_t dvd_chapter_last_cell(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

uint8_t dvd_chapter_cells(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

uint64_t dvd_chapter_blocks(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

uint64_t dvd_chapter_filesize(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

double dvd_chapter_filesize_mbs(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

#endif
