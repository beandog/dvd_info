#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_vmg_ifo.h"
#include "dvd_time.h"
#include "dvd_xchap.h"

/**
 * Clone of dvdxchap from ogmtools, but with bug fixes! :)
 */
void dvd_xchap(struct dvd_track dvd_tracks[], uint16_t track_number) {

	struct dvd_track dvd_track;
	struct dvd_chapter dvd_chapter;
	uint32_t chapter_msecs = 0;
	uint8_t chapter_number = 1;
	char chapter_start[DVD_CHAPTER_LENGTH + 1] = {'\0'};
	uint8_t c = 0;

	// dvd_xchap format starts with a single chapter that begins
	// at zero milliseconds. Next, the only chapters that are displayed
	// are the ones that are not the final one, meaning there is no chapter
	// marker to jump to the very end of a file.
	// Therefore, at least 2 chapters must exist, since the last one is the
	// stopping point, and there needs to be something in the middle.

	printf("CHAPTER01=00:00:00.000\n");
	printf("CHAPTER01NAME=Chapter 01\n");

	dvd_track = dvd_tracks[track_number - 1];

	if(dvd_track.valid == 0 && dvd_track.chapters > 1) {

		for(c = 0; c < dvd_track.chapters - 1; c++) {

			// Offset indexing by two, since the first chapter here is the starting point of the video
			chapter_number = c + 2;

			dvd_chapter = dvd_track.dvd_chapters[c];

			chapter_msecs += dvd_chapter.msecs;
			strncpy(chapter_start, milliseconds_length_format(chapter_msecs), DVD_CHAPTER_LENGTH);

			printf("CHAPTER%02u=%s\n", chapter_number, chapter_start);
			printf("CHAPTER%02uNAME=Chapter %02u\n", chapter_number, chapter_number);

		}

	}

}
