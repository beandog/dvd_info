#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

void usage(void);

void usage() {
	fprintf(stderr, "Usage: dvd_num_ifos [dvd path]\n");
}

int main(int argc, char **argv) {

	char *dvd;
	uint16_t num_ifos;

	dvd = argv[1];

	if(argc == 1) {
		dvd = "/dev/dvd";
	} else if(argc == 2) {
		dvd = argv[1];
	} else {
		usage();
		return 1;
	}

	dvd_reader_t *dvdread_dvd;
	dvdread_dvd = DVDOpen(dvd);

	if(!dvdread_dvd) {
		printf("dvd_qa_ifo: DVDOpen(%s) died!\n", dvd);
		return 1;
	}

	// Open IFO zero
	ifo_handle_t *vmg_ifo;
	vmg_ifo = ifoOpen(dvdread_dvd, 0);
	if(!vmg_ifo) {
		DVDClose(dvdread_dvd);
		return 1;
	}

	num_ifos = vmg_ifo->vts_atrt->nr_of_vtss;
	ifoClose(vmg_ifo);

	printf("%d\n", num_ifos);

	DVDClose(dvdread_dvd);

	return 0;

}
