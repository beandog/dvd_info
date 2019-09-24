#ifndef DVD_INFO_TIME_H
#define DVD_INFO_TIME_H

#include "dvd_track.h"

uint32_t dvd_time_to_milliseconds(dvd_time_t *dvd_time);

void milliseconds_length_format(char *dest_str, uint32_t milliseconds);

uint32_t dvd_track_msecs(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

void dvd_track_length(char *dest_str, ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

uint32_t dvd_chapter_msecs(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

uint32_t dvd_track_total_chapter_msecs(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

void dvd_chapter_length(char *dest_str, ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t chapter_number);

uint32_t dvd_cell_msecs(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t cell_number);

void dvd_cell_length(char *dest_str, ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t cell_number);

#endif
