#include "dvd_open.h"

int device_open(const char *device_filename) {

	// Poll drive status if it is hardware
	if(dvd_device_is_hardware(device_filename)) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "DVD drive status: ");
			dvd_drive_display_status(device_filename);

			return 1;

		}

	}

	return 0;

}

dvd_reader_t *dvdread_open(const char *device_filename) {

	// Open DVD device
	dvd_reader_t *dvdread_dvd = DVDOpen(device_filename);

	if(!dvdread_dvd)
		return NULL;

	return dvdread_dvd;

}

struct dvd_info dvd_info_open(dvd_reader_t *dvdread_dvd, const char *device_filename) {

	struct dvd_info dvd_info;

	dvd_info.valid = 1;

	// Open VMG IFO -- where all the cool stuff is
	ifo_handle_t *vmg_ifo = NULL;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo)) {
		fprintf(stderr, "Opening VMG IFO failed\n");
		dvd_info.valid = 0;
		return dvd_info;
	}

	// Initialize strings
	memset(dvd_info.dvdread_id, '\0', sizeof(dvd_info.dvdread_id));
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	memset(dvd_info.provider_id, '\0', sizeof(dvd_info.provider_id));
	memset(dvd_info.vmg_id, '\0', sizeof(dvd_info.vmg_id));

	// Initialize counters
	dvd_info.valid_video_title_sets = 0;
	dvd_info.invalid_video_title_sets = 0;
	dvd_info.valid_tracks = 0;
	dvd_info.invalid_tracks = 0;

	// GRAB ALL THE THINGS
	dvd_dvdread_id(dvd_info.dvdread_id, dvdread_dvd);
	dvd_info.video_title_sets = dvd_video_title_sets(vmg_ifo);
	dvd_info.side = dvd_info_side(vmg_ifo);
	dvd_title(dvd_info.title, device_filename);
	dvd_provider_id(dvd_info.provider_id, vmg_ifo);
	dvd_vmg_id(dvd_info.vmg_id, vmg_ifo);
	dvd_info.tracks = dvd_tracks(vmg_ifo);

	return dvd_info;

}
