#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <dvdread/ifo_read.h>
#include "dvd_vmg_ifo.h"
#include "dvd_track.h"

int dvd_device_title(const char *device_filename, char *p) {

	char dvd_title[33] = {'\0'};
	FILE *filehandle = 0;
	size_t x;
	size_t y;

	// If we can't even open the device, exit quietly
	filehandle = fopen(device_filename, "r");
	if(filehandle == NULL) {
		return 1;
	}

	// The DVD title is actually on the disc, and doesn't need the dvdread
	// or dvdnav library to access it.  I should prefer to use them, though
	// to avoid situations where something freaks out for not decrypting
	// the CSS first ... so, I guess a FIXME is in order -- or use decss
	// first.
	if(fseek(filehandle, 32808, SEEK_SET) == -1) {
		fclose(filehandle);
		return 2;
	}

	x = fread(dvd_title, 1, 32, filehandle);
	dvd_title[32] = '\0';
	if(x == 0) {
		fclose(filehandle);
		return 3;
	}

	fclose(filehandle);

	// A nice way to trim the string. :)
	// FIXME: could use an rtrim function in general
	y = strlen(dvd_title);
	while(y-- > 2) {
		if(dvd_title[y] == ' ') {
			dvd_title[y] = '\0';
		}
	}

	strncpy(p, dvd_title, 32);

	return 0;

}

uint16_t dvd_info_num_tracks(const ifo_handle_t *ifo) {

	return ifo->tt_srpt->nr_of_srpts;

}

uint16_t dvd_info_num_vts(const ifo_handle_t *ifo) {

	return ifo->vts_atrt->nr_of_vtss;
}

const char *dvd_info_provider_id(const ifo_handle_t *ifo) {

	return strndup(ifo->vmgi_mat->provider_identifier, 32);

}

const char *dvd_info_vmg_id(const ifo_handle_t *ifo) {

	return strndup(ifo->vmgi_mat->vmg_identifier, 12);

}

uint8_t dvd_info_side(const ifo_handle_t *ifo) {

	if(ifo->vmgi_mat->disc_side == 2)
		return 2;
	else
		return 1;

}

// Requires libdvdnav
// TODO add functionality to libdvdread, as well as alternate title
/*
void dvd_info_serial_id(dvdnav_t *dvdnav, char *p) {

	const char *serial_id;
	unsigned long n;

	dvdnav_get_serial_string(dvdnav, &serial_id);

	n = strlen(serial_id);
	if(n > 0)
		strncpy(p, serial_id, 16);
	else
		memset(p, '\0', 17);

}
*/

uint16_t dvd_info_longest_track(dvd_reader_t *dvdread_dvd) {

	ifo_handle_t *vmg_ifo;
	ifo_handle_t *track_ifo;
	uint16_t tracks;
	uint16_t track;
	uint16_t longest_track;
	uint16_t idx;
	uint8_t ifo_number;
	uint32_t ms;
	uint32_t max_len;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	track = 0;
	longest_track = 0;
	ms = 0;
	max_len = 0;

	if(!vmg_ifo)
		return 0;

	tracks = vmg_ifo->tt_srpt->nr_of_srpts;

	if(tracks < 1)
		return 0;

	for(idx = 0; idx < tracks; idx++) {

		track = idx + 1;
		ifo_number = dvd_track_ifo_number(vmg_ifo, track);
		track_ifo = ifoOpen(dvdread_dvd, ifo_number);

		if(!track_ifo)
			return 0;

		ms = dvd_track_milliseconds(vmg_ifo, track_ifo, track);

		// The *first* track with the longest length will still be the
		// response.
		if(ms > max_len) {
			max_len = ms;
			longest_track = track;
		}

		ifoClose(track_ifo);
		track_ifo = NULL;

	}

	ifoClose(vmg_ifo);
	vmg_ifo = NULL;

	if(max_len == 0 || track == 0)
		return 0;

	return longest_track;

}

uint16_t dvd_info_longest_track_with_subtitles(dvd_reader_t *dvdread_dvd) {

	ifo_handle_t *vmg_ifo;
	ifo_handle_t *track_ifo;
	uint16_t tracks;
	uint16_t track;
	uint16_t longest_track;
	uint8_t subtitles;
	uint8_t max_subtitles;
	uint16_t idx;
	uint8_t ifo_number;
	uint32_t ms;
	uint32_t max_len;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	track = 0;
	longest_track = 0;
	ms = 0;
	max_len = 0;
	max_subtitles = 0;

	if(!vmg_ifo)
		return 0;

	tracks = vmg_ifo->tt_srpt->nr_of_srpts;

	if(tracks < 1)
		return 0;

	for(idx = 0; idx < tracks; idx++) {

		track = idx + 1;
		ifo_number = dvd_track_ifo_number(vmg_ifo, track);
		track_ifo = ifoOpen(dvdread_dvd, ifo_number);

		if(!track_ifo)
			return 0;

		subtitles = dvd_track_subtitles(track_ifo);

		if(subtitles > 0) {

			if(subtitles >= max_subtitles) {

				max_subtitles = subtitles;

				ms = dvd_track_milliseconds(vmg_ifo, track_ifo, track);

				// The *first* track with the longest length will still be the
				// response.
				if(ms > max_len) {
					max_len = ms;
					longest_track = track;
				}

			}

		}

		ifoClose(track_ifo);
		track_ifo = NULL;

	}

	ifoClose(vmg_ifo);
	vmg_ifo = NULL;

	if(max_len == 0 || track == 0 || max_subtitles == 0)
		return 0;

	return longest_track;

}

uint16_t dvd_info_longest_16x9_track(dvd_reader_t *dvdread_dvd) {

	ifo_handle_t *vmg_ifo;
	ifo_handle_t *track_ifo;
	uint16_t tracks;
	uint16_t track;
	uint16_t longest_track;
	bool aspect_ratio_16x9;
	uint16_t idx;
	uint8_t ifo_number;
	uint32_t ms;
	uint32_t max_len;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	track = 0;
	longest_track = 0;
	ms = 0;
	max_len = 0;

	if(!vmg_ifo)
		return 0;

	tracks = vmg_ifo->tt_srpt->nr_of_srpts;

	if(tracks < 1)
		return 0;

	for(idx = 0; idx < tracks; idx++) {

		track = idx + 1;
		ifo_number = dvd_track_ifo_number(vmg_ifo, track);
		track_ifo = ifoOpen(dvdread_dvd, ifo_number);

		if(!track_ifo)
			return 0;

		aspect_ratio_16x9 = dvd_track_aspect_ratio_16x9(track_ifo);

		if(aspect_ratio_16x9) {

			ms = dvd_track_milliseconds(vmg_ifo, track_ifo, track);

			// The *first* track with the longest length will still be the
			// response.
			if(ms > max_len) {
				max_len = ms;
				longest_track = track;
			}

		}

		ifoClose(track_ifo);
		track_ifo = NULL;

	}

	ifoClose(vmg_ifo);
	vmg_ifo = NULL;

	if(max_len == 0 || track == 0)
		return 0;

	return longest_track;

}

uint16_t dvd_info_longest_4x3_track(dvd_reader_t *dvdread_dvd) {

	ifo_handle_t *vmg_ifo;
	ifo_handle_t *track_ifo;
	uint16_t tracks;
	uint16_t track;
	uint16_t longest_track;
	bool aspect_ratio_4x3;
	uint16_t idx;
	uint8_t ifo_number;
	uint32_t ms;
	uint32_t max_len;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	track = 0;
	longest_track = 0;
	ms = 0;
	max_len = 0;

	if(!vmg_ifo)
		return 0;

	tracks = vmg_ifo->tt_srpt->nr_of_srpts;

	if(tracks < 1)
		return 0;

	for(idx = 0; idx < tracks; idx++) {

		track = idx + 1;
		ifo_number = dvd_track_ifo_number(vmg_ifo, track);
		track_ifo = ifoOpen(dvdread_dvd, ifo_number);

		if(!track_ifo)
			return 0;

		aspect_ratio_4x3 = dvd_track_aspect_ratio_4x3(track_ifo);

		if(aspect_ratio_4x3) {

			ms = dvd_track_milliseconds(vmg_ifo, track_ifo, track);

			// The *first* track with the longest length will still be the
			// response.
			if(ms > max_len) {
				max_len = ms;
				longest_track = track;
			}

		}

		ifoClose(track_ifo);
		track_ifo = NULL;

	}

	ifoClose(vmg_ifo);
	vmg_ifo = NULL;

	if(max_len == 0 || track == 0)
		return 0;

	return longest_track;

}

uint16_t dvd_info_longest_letterbox_track(dvd_reader_t *dvdread_dvd) {

	ifo_handle_t *vmg_ifo;
	ifo_handle_t *track_ifo;
	uint16_t tracks;
	uint16_t track;
	uint16_t longest_track;
	bool letterboxed;
	uint16_t idx;
	uint8_t ifo_number;
	uint32_t ms;
	uint32_t max_len;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	track = 0;
	longest_track = 0;
	ms = 0;
	max_len = 0;

	if(!vmg_ifo)
		return 0;

	tracks = vmg_ifo->tt_srpt->nr_of_srpts;

	if(tracks < 1)
		return 0;

	for(idx = 0; idx < tracks; idx++) {

		track = idx + 1;
		ifo_number = dvd_track_ifo_number(vmg_ifo, track);
		track_ifo = ifoOpen(dvdread_dvd, ifo_number);

		if(!track_ifo)
			return 0;

		letterboxed = dvd_track_letterbox_video(track_ifo);

		if(letterboxed) {

			ms = dvd_track_milliseconds(vmg_ifo, track_ifo, track);

			// The *first* track with the longest length will still be the
			// response.
			if(ms > max_len) {
				max_len = ms;
				longest_track = track;
			}

		}

		ifoClose(track_ifo);
		track_ifo = NULL;

	}

	ifoClose(vmg_ifo);
	vmg_ifo = NULL;

	if(max_len == 0 || track == 0)
		return 0;

	return longest_track;

}

uint16_t dvd_info_longest_pan_scan_track(dvd_reader_t *dvdread_dvd) {

	ifo_handle_t *vmg_ifo;
	ifo_handle_t *track_ifo;
	uint16_t tracks;
	uint16_t track;
	uint16_t longest_track;
	bool pan_and_scan;
	uint16_t idx;
	uint8_t ifo_number;
	uint32_t ms;
	uint32_t max_len;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	track = 0;
	longest_track = 0;
	ms = 0;
	max_len = 0;

	if(!vmg_ifo)
		return 0;

	tracks = vmg_ifo->tt_srpt->nr_of_srpts;

	if(tracks < 1)
		return 0;

	for(idx = 0; idx < tracks; idx++) {

		track = idx + 1;
		ifo_number = dvd_track_ifo_number(vmg_ifo, track);
		track_ifo = ifoOpen(dvdread_dvd, ifo_number);

		if(!track_ifo)
			return 0;

		pan_and_scan = dvd_track_pan_scan_video(track_ifo);

		if(pan_and_scan) {

			ms = dvd_track_milliseconds(vmg_ifo, track_ifo, track);

			// The *first* track with the longest length will still be the
			// response.
			if(ms > max_len) {
				max_len = ms;
				longest_track = track;
			}

		}

		ifoClose(track_ifo);
		track_ifo = NULL;

	}

	ifoClose(vmg_ifo);
	vmg_ifo = NULL;

	if(max_len == 0 || track == 0)
		return 0;

	return longest_track;

}
