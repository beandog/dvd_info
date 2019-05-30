#include "dvd_ogm.h"

/**
 * Clone of dvdxchap from ogmtools, but with bug fixes! :)
 */
void dvd_ogm(struct dvd_track dvd_track) {

	// dvd_xchap format starts with a single chapter that begins
	// at zero milliseconds. Next, the only chapters that are displayed
	// are the ones that are not the final one, meaning there is no chapter
	// marker to jump to the very end of a file.
	// Therefore, at least 2 chapters must exist, since the last one is the
	// stopping point, and there needs to be something in the middle.

	printf("CHAPTER01=00:00:00.000\n");
	printf("CHAPTER01NAME=Chapter 01\n");

	if(dvd_track.valid && dvd_track.chapters > 1) {

		struct dvd_chapter dvd_chapter;
		uint32_t chapter_msecs = 0;
		uint8_t chapter_number = 1;
		char chapter_start[DVD_CHAPTER_LENGTH + 1] = {'\0'};
		uint8_t c = 0;

		for(c = 0; c < dvd_track.chapters - 1; c++) {

			// Offset indexing by two, since the first chapter here is the starting point of the video
			chapter_number = c + 2;

			dvd_chapter = dvd_track.dvd_chapters[c];

			chapter_msecs += dvd_chapter.msecs;
			milliseconds_length_format(chapter_start, chapter_msecs);

			printf("CHAPTER%02" PRIu8 "=%s\n", chapter_number, chapter_start);
			printf("CHAPTER%02" PRIu8 "NAME=Chapter %02" PRIu8 "\n", chapter_number, chapter_number);

		}

	}

}
