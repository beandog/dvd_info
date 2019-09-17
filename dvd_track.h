#ifndef DVD_INFO_TRACK_H
#define DVD_INFO_TRACK_H

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "dvd_vmg_ifo.h"
#include "dvd_time.h"
#include "dvd_video.h"

struct dvd_video {
	char codec[DVD_VIDEO_CODEC + 1];
	char format[DVD_VIDEO_FORMAT + 1];
	char aspect_ratio[DVD_VIDEO_ASPECT_RATIO + 1];
	uint16_t width;
	uint16_t height;
	bool letterbox;
	bool pan_and_scan;
	uint8_t df;
	char fps[DVD_VIDEO_FPS + 1];
	uint8_t angles;
};

struct dvd_track {
	uint16_t track;
	bool valid;
	uint16_t vts;
	uint8_t ttn;
	char length[DVD_TRACK_LENGTH + 1];
	uint32_t msecs;
	uint8_t chapters;
	uint8_t audio_tracks;
	uint8_t subtitles;
	uint8_t cells;
	bool min_sector_error;
	bool max_sector_error;
	bool repeat_first_sector_error;
	bool repeat_last_sector_error;
	struct dvd_video dvd_video;
	struct dvd_audio *dvd_audio_tracks;
	uint8_t active_audio_streams;
	struct dvd_subtitle *dvd_subtitles;
	uint8_t active_subs;
	struct dvd_chapter *dvd_chapters;
	struct dvd_cell *dvd_cells;
	uint64_t blocks;
	uint64_t filesize;
	double filesize_mbs;
};

typedef struct {
	uint16_t track;
	bool valid;
	uint16_t vts;
	uint8_t ttn;
	char length[DVD_TRACK_LENGTH + 1];
	uint32_t msecs;
	uint8_t chapters;
	uint8_t audio_tracks;
	uint8_t subtitles;
	uint8_t cells;
	uint8_t active_audio_streams;
	uint8_t active_subs;
	uint64_t blocks;
	uint64_t filesize;
	double filesize_mbs;
} dvd_track_t;

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

typedef struct {
	uint8_t cell;
	char length[DVD_CELL_LENGTH + 1];
	uint32_t msecs;
	uint64_t first_sector;
	uint64_t last_sector;
	uint64_t blocks;
	uint64_t filesize;
	double filesize_mbs;
} dvd_cell_t;

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

uint32_t dvd_cell_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number);

void dvd_cell_length(char *dest_str, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number);

dvd_cell_t *dvd_cell_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t cell_number);

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

uint16_t dvd_vts_ifo_number(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

uint8_t dvd_track_ttn(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

uint8_t dvd_track_chapters(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

uint8_t dvd_track_cells(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

uint64_t dvd_track_blocks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

uint64_t dvd_track_filesize(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

double dvd_track_filesize_mbs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

uint8_t dvd_track_audio_tracks(const ifo_handle_t *vts_ifo);

uint8_t dvd_audio_active_tracks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

uint8_t dvd_track_subtitles(const ifo_handle_t *vts_ifo);

uint8_t dvd_track_active_subtitles(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

uint32_t dvd_track_msecs(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

void dvd_track_length(char *dest_str, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

dvd_track_t *dvd_track_init(dvd_reader_t *dvdread_dvd, ifo_handle_t *vmg_ifo, uint16_t track);

#endif
