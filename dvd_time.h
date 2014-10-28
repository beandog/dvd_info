#ifndef _DVD_TIME_H_
#define _DVD_TIME_H_

/**
 * Convert any value of dvd_time to milliseconds
 *
 * @param dvd_time dvd_time
 */
uint32_t dvd_time_to_milliseconds(dvd_time_t *dvd_time);

/**
 * Convert milliseconds to format hh:mm:ss.ms
 *
 * @param milliseconds milliseconds
 */
const char *milliseconds_length_format(const uint32_t milliseconds);

/**
 * Get the number of milliseconds for a title track
 */
uint32_t dvd_track_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

/**
 * Get the formatted string length of a title track
 */
const char *dvd_track_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

/**
 * Get the number of milliseconds of a chapter
 */
uint32_t dvd_chapter_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

/**
 * Get the formatted string length of a chapter
 */
const char *dvd_chapter_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t chapter_number);

/**
 * Get the number of milliseconds of a cell
 */
uint32_t dvd_cell_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number);

/**
 * Get the formatted string length of a cell
 */
const char *dvd_cell_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number);

#endif
