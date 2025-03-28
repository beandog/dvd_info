#include "dvd_open.h"

/**
  * By default, libdvdread prints out a lot of messages, and sends to stdout.
  * This callback function captures the log messages, and sends them to stderr
  * instead. Also, only interesting logs should be spit out, ones that indicate
  * that something is probably very wrong with the source.
  *
  * libdvdread error levels:
  * DVD_LOGGER_LEVEL_INFO
  * DVD_LOGGER_LEVEL_ERROR
  * DVD_LOGGER_LEVEL_WARN
  * DVD_LOGGER_LEVEL_DEBUG (original default)
  *
  * DVD_LOGGER_LEVEL_INFO examples:
  * libdvdread: Attempting to retrieve all CSS keys
  * libdvdread: This can take a _long_ time, please be patient
  *
  * DVD_LOGGER_LEVEL_ERROR is set when there are problems with memory or
  * with libdvdcss or file can't be opened (see libdvdread/src/dvd_input.c)
  *
  * DVD_LOGGER_LEVEL_WARN examples:
  * libdvdread: Error cracking CSS key for /VIDEO_TS/VTS_03_0.VOB (0x0000047a)
  * libdvdread: CHECK_VALUE failed in libdvdread-6.1.3/src/ifo_read.c:917 for
  *   pgc->cell_playback_offset != 0
  *
  * DVD_LOGGER_LEVEL_DEBUG examples:
  * libdvdread: Get key for /VIDEO_TS/VIDEO_TS.VOB at 0x000001fe
  * libdvdread: Elapsed time 0
  * libdvdread: Found 18 VTS's
  *
  */

bool log_verbose = false;
bool log_debug = false;

void dvd_info_logger_cb(void *p, dvd_logger_level_t dvdread_log_level, const char *msg, va_list dvd_log_va) {

	char dvd_log[2048];

	memset(dvd_log, '\0', sizeof(dvd_log));

	vsnprintf(dvd_log, sizeof(dvd_log), msg, dvd_log_va);

	if(log_verbose) {
		if(dvdread_log_level == DVD_LOGGER_LEVEL_INFO)
			fprintf(stderr, "[INFO] libdvdread: %s\n", dvd_log);
		else if(dvdread_log_level == DVD_LOGGER_LEVEL_WARN)
			fprintf(stderr, "[WARNING] libdvdread: %s\n", dvd_log);
		else if(dvdread_log_level == DVD_LOGGER_LEVEL_ERROR)
			fprintf(stderr, "[ERROR] libdvdread: %s\n", dvd_log);
	}

	if(log_debug && dvdread_log_level == DVD_LOGGER_LEVEL_DEBUG) {
		fprintf(stderr, "[DEBUG] libdvdread: %s\n", dvd_log);
	}

}

int device_open(const char *device_filename) {

#ifdef __linux__

	bool hardware = false;

	if(strncmp(device_filename, "/dev/", 5) == 0)
		hardware = true;

	// Poll drive status if it is hardware
	if(hardware) {

		// Wait for the drive to become ready
		if(!dvd_drive_has_media(device_filename)) {

			fprintf(stderr, "DVD drive status: ");
			dvd_drive_display_status(device_filename);

			return 1;

		}

	}

#endif

	return 0;

}

struct dvd_info dvd_info_open(dvd_reader_t *dvdread_dvd, const char *device_filename) {

	struct dvd_info dvd_info;

	dvd_info.valid = 1;

	// Open VMG IFO -- where all the cool stuff is
	ifo_handle_t *vmg_ifo = NULL;

	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || vmg_ifo->vmgi_mat == NULL) {
		fprintf(stderr, "Opening VMG IFO failed\n");
		dvd_info.valid = 0;
		return dvd_info;
	}

	// Initialize strings
	memset(dvd_info.dvdread_id, '\0', sizeof(dvd_info.dvdread_id));
	memset(dvd_info.title, '\0', sizeof(dvd_info.title));
	memset(dvd_info.alternative_title, '\0', sizeof(dvd_info.alternative_title));
	memset(dvd_info.serial_number, '\0', sizeof(dvd_info.serial_number));
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
	// FIXME these are for debugging right now, since it adds opening the DVD two more times.
		dvd_alternative_title(dvd_info.alternative_title, device_filename);
		dvd_serial_number(dvd_info.serial_number, device_filename);
	dvd_provider_id(dvd_info.provider_id, vmg_ifo);
	dvd_vmg_id(dvd_info.vmg_id, vmg_ifo);
	dvd_info.tracks = dvd_tracks(vmg_ifo);

	// FIXME either put longest_track in here, or remove it from the struct

	return dvd_info;

}
