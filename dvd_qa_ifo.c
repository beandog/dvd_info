#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

/**
 * Determine if an IFO is valid, by checking to see if dvdread's ifoOpen()
 * function can access it and parse the data.
 *
 * @retval 0 valid IFO
 * @retval 1 invalid IFO
 * @retval 2 Invalid program syntax
 * @retval 3 Could not open DVD
 * @retval 4 Could not open IFO 0
 */
int main(int, char **);

void usage() {
	printf("DVD QA - Check for invalid IFO\n");
	printf("Usage: dvd_qa_ifo [-i dvd path] [-n IFO number]\n");
}

int main(int argc, char **argv) {

	char *dvd_path = NULL;
	int dvd_title_sets;
	int dvd_ifo_number;
	dvd_reader_t *dvd_reader;
	ifo_handle_t *dvd_ifo;
	int opt;
	opterr = 0;

	int hflag = 0;
	int iflag = 0;
	int nflag = 0;
	char options[] = "hi:n:";

	while((opt = getopt(argc, argv, options)) != -1) {

		switch(opt) {

			case 'h':
				hflag = 1;
				break;

			case 'i':
				dvd_path = optarg;
				iflag = 1;
				break;

			case 'n':
				dvd_ifo_number = atoi(optarg);
				nflag = 1;
				break;

		}

	}

	if(hflag == 1 || iflag == 0 || nflag == 0) {
		usage();
		return 2;
	}

	dvd_reader = DVDOpen(dvd_path);
	if(!dvd_reader) {
		return 3;
	}

	if(dvd_ifo_number < 0 || dvd_ifo_number > 99) {
		printf("Invalid IFO number %i\n", dvd_ifo_number);
		DVDClose(dvd_reader);
		return 1;
	}

	// Open IFO zero
	dvd_ifo = ifoOpen(dvd_reader, 0);
	if(!dvd_ifo) {
		fprintf(stderr, "Could not open IFO 0\n");
		DVDClose(dvd_reader);
		return 4;
	} else if(dvd_ifo_number == 0) {
		ifoClose(dvd_ifo);
		DVDClose(dvd_reader);
		return 0;
	}

	dvd_title_sets = dvd_ifo->vts_atrt->nr_of_vtss;
	ifoClose(dvd_ifo);

	if(dvd_ifo_number > dvd_title_sets) {
		fprintf(stderr, "IFO number %i exceeds maximum number of IFOS, %i\n", dvd_ifo_number, dvd_title_sets);
		DVDClose(dvd_reader);
		return 2;
	} else if((dvd_ifo_number < 0) || (dvd_ifo_number > 99)) {
		fprintf(stderr, "IFO number must be between 0 and 99\n");
		DVDClose(dvd_reader);
		return 2;
	}

	dvd_ifo = ifoOpen(dvd_reader, dvd_ifo_number);

	if(dvd_ifo) {
		printf("pass\n");
		ifoClose(dvd_ifo);
		DVDClose(dvd_reader);
		return 0;
	} else {
		printf("fail\n");
		DVDClose(dvd_reader);
		return 1;
	}

}
