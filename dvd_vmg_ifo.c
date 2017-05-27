#include "dvd_vmg_ifo.h"

const char *dvd_title(const char *device_filename) {

	char dvd_title[DVD_TITLE + 1] = {'\0'};
	FILE *filehandle = 0;
	size_t x = 0;
	size_t y = 0;

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

	int dvdread_retval = 0;
	uint8_t dvdread_ifo_md5[16] = {0};
	char dvdread_id[DVD_DVDREAD_ID + 1] = {'\0'};
	unsigned long x = 0;

	// DVDDiscID will open the VMG IFO
	dvdread_retval = DVDDiscID(dvdread_dvd, dvdread_ifo_md5);
	if(dvdread_retval == -1)
		return "";

	for(x = 0; x < (DVD_DVDREAD_ID / 2); x++)
		snprintf(&dvdread_id[x * 2], DVD_DVDREAD_ID + 1, "%02x", dvdread_ifo_md5[x]);

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

const char *dvd_specification_version(const ifo_handle_t *vmg_ifo) {

	if(ifo_is_vmg(vmg_ifo)) {
		char str[DVD_SPECIFICATION_VERSION + 1] = {'\0'};
		snprintf(str, DVD_SPECIFICATION_VERSION + 1, "%01x.%01x", vmg_ifo->vmgi_mat->specification_version >> 4, vmg_ifo->vmgi_mat->specification_version & 0xf);
		return strndup(str, DVD_SPECIFICATION_VERSION + 1);
	} else
		return "";

}

// Requires libdvdnav
// TODO add functionality to libdvdread, as well as alternate title
/*
void dvd_info_serial_id(dvdnav_t *dvdnav, char *p) {

	const char *serial_id;
	size_t n;

	dvdnav_get_serial_string(dvdnav, &serial_id);

	n = strlen(serial_id);
	if(n > 0)
		strncpy(p, serial_id, 16);
	else
		memset(p, '\0', 17);

}
*/
