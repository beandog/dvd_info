#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_vmg_ifo.h"
#include "dvd_track.h"

const char *dvd_title(const char *device_filename) {

	char dvd_title[DVD_TITLE + 1] = {'\0'};
	FILE *filehandle = 0;
	size_t x;
	size_t y;

	// If we can't even open the device, exit quietly
	filehandle = fopen(device_filename, "r");
	if(filehandle == NULL) {
		return "";
	}

	// The DVD title is on the volume, and doesn't need the dvdread or
	// dvdnav library to access it.
	if(fseek(filehandle, 32808, SEEK_SET) == -1) {
		fclose(filehandle);
		return "";
	}

	x = fread(dvd_title, 1, DVD_TITLE, filehandle);
	dvd_title[DVD_TITLE] = '\0';
	if(x == 0) {
		fclose(filehandle);
		return "";
	}

	fclose(filehandle);

	// Right trim the string
	y = strlen(dvd_title);
	while(y-- > 2) {
		if(dvd_title[y] == ' ') {
			dvd_title[y] = '\0';
		}
	}

	return strndup(dvd_title, DVD_TITLE);

}

const char *dvd_dvdread_id(dvd_reader_t *dvdread_dvd) {

	int dvdread_retval;
	uint8_t dvdread_ifo_md5[16] = {0};
	char dvdread_id[DVD_DVDREAD_ID + 1] = {'\0'};
	unsigned long x;

	dvdread_retval = DVDDiscID(dvdread_dvd, dvdread_ifo_md5);
	if(dvdread_retval == -1)
		return "";

	for(x = 0; x < (DVD_DVDREAD_ID / 2); x++)
		sprintf(&dvdread_id[x * 2], "%02x", dvdread_ifo_md5[x]);

	return strndup(dvdread_id, DVD_DVDREAD_ID);

}

bool ifo_is_vmg(const ifo_handle_t *ifo) {

	if(ifo->vmgi_mat == NULL)
		return false;

	return true;

}

uint16_t dvd_tracks(const ifo_handle_t *vmg_ifo) {

	if(ifo_is_vmg(vmg_ifo) && vmg_ifo->tt_srpt != NULL)
		return vmg_ifo->tt_srpt->nr_of_srpts;
	else
		return 0;

}

uint16_t dvd_video_title_sets(const ifo_handle_t *vmg_ifo) {

	if(ifo_is_vmg(vmg_ifo) && vmg_ifo->vts_atrt != NULL)
		return vmg_ifo->vts_atrt->nr_of_vtss;
	else
		return 0;
}

const char *dvd_provider_id(const ifo_handle_t *vmg_ifo) {

	if(ifo_is_vmg(vmg_ifo))
		return strndup(vmg_ifo->vmgi_mat->provider_identifier, DVD_PROVIDER_ID);
	else
		return "";

}

const char *dvd_vmg_id(const ifo_handle_t *vmg_ifo) {

	if(ifo_is_vmg(vmg_ifo))
		return strndup(vmg_ifo->vmgi_mat->vmg_identifier, DVD_VMG_ID);
	else
		return "";

}

uint8_t dvd_info_side(const ifo_handle_t *vmg_ifo) {

	if(ifo_is_vmg(vmg_ifo)) {
		if(vmg_ifo->vmgi_mat->disc_side == 2)
			return 2;
		else
			return 1;
	} else {
		return 1;
	}

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

uint16_t dvd_longest_track(dvd_reader_t *dvdread_dvd) {

	ifo_handle_t *vmg_ifo;
	ifo_handle_t *vts_ifo;
	uint16_t tracks;
	uint16_t track;
	uint16_t longest_track;
	uint16_t idx;
	uint16_t ifo_number;
	uint32_t ms;
	uint32_t max_len;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	track = 0;
	longest_track = 0;
	ms = 0;
	max_len = 0;

	if(!vmg_ifo)
		return 0;

	if(!ifo_is_vmg(vmg_ifo) && vmg_ifo->tt_srpt != NULL)
		return 0;

	tracks = vmg_ifo->tt_srpt->nr_of_srpts;

	if(tracks < 1)
		return 0;

	for(idx = 0; idx < tracks; idx++) {

		track = idx + 1;
		ifo_number = dvd_vts_ifo_number(vmg_ifo, track);
		vts_ifo = ifoOpen(dvdread_dvd, ifo_number);

		if(!vts_ifo)
			continue;

		ms = dvd_track_milliseconds(vmg_ifo, vts_ifo, track);

		// The *first* track with the longest length will still be the
		// response.
		if(ms > max_len) {
			max_len = ms;
			longest_track = track;
		}

		ifoClose(vts_ifo);
		vts_ifo = NULL;

	}

	ifoClose(vmg_ifo);
	vmg_ifo = NULL;

	if(max_len == 0 || track == 0)
		return 0;

	return longest_track;

}
