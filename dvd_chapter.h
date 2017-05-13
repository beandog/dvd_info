#ifndef _DVD_CHAPTER_H_
#define _DVD_CHAPTER_H_

#include "dvd_info.h"
#include "dvd_cell.h"

uint8_t dvd_chapter_first_cell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

uint8_t dvd_chapter_last_cell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

#endif
