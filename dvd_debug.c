#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>
#include "dvd_device.h"

int main(int argc, char **argv) {

	const char *device_filename = NULL;
	if (argc == 1)
		device_filename = argv[1];
	else
		device_filename = DEFAULT_DVD_DEVICE;

	/** Begin dvd_debug :) */

	// begin libdvdread usage

	// Open DVD device
	dvd_reader_t *dvdread_dvd = NULL;
	dvdread_dvd = DVDOpen(device_filename);
	if(!dvdread_dvd) {
		fprintf(stderr, "Opening DVD %s failed\n", device_filename);
		return 1;
	}

	uint16_t vts = 1;

	ifo_handle_t *vmg_ifo = NULL;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);

	if(vmg_ifo == NULL || vmg_ifo->vmgi_mat == NULL) {
		fprintf(stderr, "Could not open VMG IFO\n");
		DVDClose(dvdread_dvd);
		return 1;
	}

	printf("[dvd_debug: VMG IFO]\n");
	ifo_print(dvdread_dvd, 0);

	ifo_handle_t *vts_ifo = NULL;
	for(vts = 1; vts < vmg_ifo->vts_atrt->nr_of_vtss + 1; vts++) {

		vts_ifo = ifoOpen(dvdread_dvd, vts);

		if(vts_ifo == NULL) {
			fprintf(stderr, "Opening VTS IFO %" PRIu16 " failed, skipping\n", vts);
			continue;
		}

		printf("[dvd_debug: VTS IFO %" PRIu16 "]\n", vts);
		ifo_print(dvdread_dvd, vts);

	}

	return 0;

}
