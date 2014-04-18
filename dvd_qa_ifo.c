#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

void usage() {
	printf("Usage: dvd_qa_ifo [dvd path] [IFO number]\n");
}

int main(int argc, char **argv) {

	char *program;
	char *dvd;
	int num_title_sets;
	int i;
	int ifo_number;

	program = argv[0];
	dvd = argv[1];

	if(argc == 1) {
		dvd = "/dev/dvd";
		ifo_number = 0;
	} else if(argc == 2) {
		dvd = argv[1];
		ifo_number = 0;
	} else if(argc == 3) {
		dvd = argv[1];
		ifo_number = atoi(argv[2]);
		if(ifo_number < 0 || ifo_number > 99) {
			printf("Invalid IFO number %i\n", ifo_number);
			return 1;
		}
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
	ifo_handle_t *ifo_zero;
	ifo_zero = ifoOpen(dvdread_dvd, 0);
	if(!ifo_zero) {
		DVDClose(dvdread_dvd);
		return 1;
	}

	num_title_sets = ifo_zero->vmgi_mat->vmg_nr_of_title_sets;
	ifoClose(ifo_zero);

	if(ifo_number > num_title_sets) {
		printf("Invalid IFO number %i\n", ifo_number);
		printf("num ifos: %d\n", num_title_sets);
		return 1;
	}

	ifo_handle_t *ifo;

	for (i = 0; i < num_title_sets; i++) {
		ifo = ifoOpen(dvdread_dvd, i);
		if(!ifo) {
			DVDClose(dvdread_dvd);
			return 1;
		}
	}

	DVDClose(dvdread_dvd);

	return 0;

}
