#include "dvd_vmg_ifo.h"

bool ifo_is_vts(ifo_handle_t *ifo) {

	if(ifo->vtsi_mat == NULL)
		return false;
	else
		return true;

}

bool dvd_title(char *dest_str, const char *device_filename) {

	char dvd_title[DVD_TITLE + 1] = {'\0'};
	int fd = -1;

	// If we can't even open the device, exit quietly
	fd = open(device_filename, O_RDONLY);
	if(fd == -1) {
		return false;
	}

	// The DVD title is on the volume, and doesn't need the dvdread or
	// dvdnav library to access it.
	if(lseek(fd, 32768, SEEK_SET) != 32768) {
		close(fd);
		return false;
	}

	char buffer[2048] = {'\0'};

	if(read(fd, buffer, 2048) != 2048) {
		close(fd);
		return false;
	}
	snprintf(dvd_title, DVD_TITLE, "%s", buffer + 40);

	close(fd);

	// Right trim the string
	size_t dvd_title_length = 0;
	dvd_title_length = strlen(dvd_title);
	while(dvd_title_length-- > 2) {
		if(dvd_title[dvd_title_length] == ' ') {
			dvd_title[dvd_title_length] = '\0';
		}
	}

	strncpy(dest_str, dvd_title, DVD_TITLE);

	return true;

}

bool dvd_alternative_title(char *dest_str, const char *device_filename) {

	char dvd_alternative_title[DVD_TITLE + 1] = {'\0'};
	int fd = -1;

	// If we can't even open the device, exit quietly
	fd = open(device_filename, O_RDONLY);
	if(fd == -1) {
		return false;
	}

	// The DVD alternative title is on the volume, and doesn't need the dvdread or
	// dvdnav library to access it.
	if(lseek(fd, 65536, SEEK_SET) != 65536) {
		close(fd);
		return false;
	}

	char buffer[DVD_VIDEO_LB_LEN] = {'\0'};

	if(read(fd, buffer, DVD_VIDEO_LB_LEN) != DVD_VIDEO_LB_LEN) {
		close(fd);
		return false;
	}
	snprintf(dvd_alternative_title, DVD_TITLE, "%s", buffer + 40);

	close(fd);

	// Right trim the string
	size_t dvd_title_length = 0;
	dvd_title_length = strlen(dvd_alternative_title);
	while(dvd_title_length-- > 2) {
		if(dvd_alternative_title[dvd_title_length] == ' ') {
			dvd_alternative_title[dvd_title_length] = '\0';
		}
	}

	strncpy(dest_str, dvd_alternative_title, DVD_TITLE);

	return true;

}

bool dvd_serial_number(char *dest_str, const char *device_filename) {

	char dvd_serial[DVD_SERIAL_NUMBER + 1] = {'\0'};
	int fd = -1;

	// If we can't even open the device, exit quietly
	fd = open(device_filename, O_RDONLY);
	if(fd == -1) {
		return false;
	}

	// The DVD serial number is on the volume, and doesn't need the dvdread or
	// dvdnav library to access it.
	if(lseek(fd, 65536 + DVD_TITLE, SEEK_SET) != (65536 + DVD_TITLE)) {
		printf("serial: lseek failed!\n");
		close(fd);
		return false;
	}

	char buffer[DVD_VIDEO_LB_LEN] = {'\0'};

	if(read(fd, buffer, DVD_VIDEO_LB_LEN) != DVD_VIDEO_LB_LEN) {
		close(fd);
		return false;
	}
	// FIXME for some reason this first character is deleting the previous one when using offset of 40??
	// Look at libdvndav/src/vm/vm.c dvd_read_name()
	snprintf(dvd_serial, DVD_SERIAL_NUMBER + 1, "%s", buffer + 41);

	close(fd);

	// Right trim the string
	// FIXME might not need trimming
	size_t dvd_serial_str_length = 0;
	dvd_serial_str_length = strlen(dvd_serial);
	while(dvd_serial_str_length-- > 2) {
		if(dvd_serial[dvd_serial_str_length] == ' ') {
			dvd_serial[dvd_serial_str_length] = '\0';
		}
	}

	strncpy(dest_str, dvd_serial, DVD_SERIAL_NUMBER + 1);

	return true;

}

bool dvd_dvdread_id(char *dest_str, dvd_reader_t *dvdread_dvd) {

	int dvdread_retval = 0;
	uint8_t dvdread_ifo_md5[16] = {0};
	char dvdread_id[DVD_DVDREAD_ID + 1] = {'\0'};
	unsigned long x = 0;

	// DVDDiscID will open the VMG IFO
	dvdread_retval = DVDDiscID(dvdread_dvd, dvdread_ifo_md5);
	if(dvdread_retval == -1)
		return false;

	// Set character length to the correct size
	// https://github.com/beandog/dvd_info/issues/12
	for(x = 0; x < (DVD_DVDREAD_ID / 2); x++)
		snprintf(&dvdread_id[x * 2], DVD_DVDREAD_MD5SUM_CHAR + 1, "%02x", dvdread_ifo_md5[x]);

	strncpy(dest_str, dvdread_id, DVD_DVDREAD_ID);

	return true;

}

bool ifo_is_vmg(ifo_handle_t *ifo) {

	if(ifo->vmgi_mat == NULL)
		return false;

	return true;

}

uint16_t dvd_tracks(ifo_handle_t *vmg_ifo) {

	if(vmg_ifo != NULL && vmg_ifo->tt_srpt != NULL)
		return vmg_ifo->tt_srpt->nr_of_srpts;
	else
		return 0;

}

uint16_t dvd_video_title_sets(ifo_handle_t *vmg_ifo) {

	if(vmg_ifo != NULL && vmg_ifo->vts_atrt != NULL)
		return vmg_ifo->vts_atrt->nr_of_vtss;
	else
		return 0;
}

bool dvd_provider_id(char *dest_str, ifo_handle_t *vmg_ifo) {

	if(vmg_ifo != NULL && vmg_ifo->vmgi_mat != NULL)
		strncpy(dest_str, vmg_ifo->vmgi_mat->provider_identifier, DVD_PROVIDER_ID);

	return true;

}

bool dvd_vmg_id(char *dest_str, ifo_handle_t *vmg_ifo) {

	if(vmg_ifo != NULL && vmg_ifo->vmgi_mat != NULL)
		strncpy(dest_str, vmg_ifo->vmgi_mat->vmg_identifier, DVD_VMG_ID);

	return true;

}

uint8_t dvd_info_side(ifo_handle_t *vmg_ifo) {

	if(vmg_ifo != NULL && vmg_ifo->vmgi_mat != NULL) {
		if(vmg_ifo->vmgi_mat->disc_side == 2)
			return 2;
		else
			return 1;
	} else {
		return 1;
	}

}

bool dvd_specification_version(char *dest_str, ifo_handle_t *vmg_ifo) {

	if(vmg_ifo != NULL && vmg_ifo->vmgi_mat != NULL) {
		snprintf(dest_str, DVD_SPECIFICATION_VERSION + 1, "%01x.%01x", vmg_ifo->vmgi_mat->specification_version >> 4, vmg_ifo->vmgi_mat->specification_version & 0xf);
		return true;
	}

	return false;

}
