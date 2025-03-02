#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>
#include "dvd_device.h"
#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif

int main(int argc, char **argv) {

	char device_filename[PATH_MAX];
	memset(device_filename, '\0', PATH_MAX);
	if (argc == 2)
		strncpy(device_filename, argv[1], PATH_MAX - 1);
	else
		strncpy(device_filename, DEFAULT_DVD_DEVICE, PATH_MAX - 1);

	/** Begin dvd_debug :) */

	// begin libdvdread usage

	// Open DVD device
	dvd_reader_t *dvdread_dvd = NULL;
	dvdread_dvd = DVDOpen(device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "[dvd_debug] Opening DVD %s failed\n", device_filename);
		return 1;
	}

	uint16_t vts = 1;

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || vmg_ifo->vmgi_mat == NULL) {
		fprintf(stderr, "[dvd_debug] Could not open VMG IFO\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	printf("[dvd_debug: VMG IFO]\n");
	ifo_print(dvdread_dvd, 0);

	ifo_handle_t *vts_ifo = NULL;

	if(vmg_ifo->vts_atrt == NULL) {
		fprintf(stderr, "[dvd_debug] DVD has no Video Title Sets\n");
		return 1;
	}

	for(vts = 1; vts < vmg_ifo->vts_atrt->nr_of_vtss + 1; vts++) {

		vts_ifo = ifoOpen(dvdread_dvd, vts);

		if(vts_ifo == NULL) {
			fprintf(stderr, "[dvd_debug] Opening VTS IFO %" PRIu16 " failed, skipping\n", vts);
			continue;
		}

		printf("[dvd_debug] VTS IFO %" PRIu16 "]\n", vts);
		ifo_print(dvdread_dvd, vts);

	}

	return 0;

}
