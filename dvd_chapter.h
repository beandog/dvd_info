#ifndef DVD_INFO_CHAPTER_H
#define DVD_INFO_CHAPTER_H

#include "dvd_chapter.h"

struct dvd_chapter {
	uint8_t chapter;
	char length[DVD_CHAPTER_LENGTH + 1];
	uint32_t msecs;
	uint8_t first_cell;
	uint8_t last_cell;
};

typedef struct {
	uint8_t chapter;
	char length[DVD_CHAPTER_LENGTH + 1];
	uint32_t msecs;
	uint8_t first_cell;
	uint8_t last_cell;
} dvd_chapter_t;

uint8_t dvd_chapter_first_cell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

uint8_t dvd_chapter_last_cell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

uint32_t dvd_chapter_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

uint32_t dvd_track_total_chapter_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

void dvd_chapter_length(char *dest_str, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t chapter_number);

dvd_chapter_t *dvd_chapter_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

#endif
