#include "dvd_json.h"

void dvd_json(struct dvd_info dvd_info, struct dvd_track dvd_tracks[], uint16_t track_number, uint16_t d_first_track, uint16_t d_last_track) {

	struct dvd_track dvd_track;
	struct dvd_video dvd_video;
	struct dvd_audio dvd_audio;
	struct dvd_subtitle dvd_subtitle;
	struct dvd_chapter dvd_chapter;
	struct dvd_cell dvd_cell;
	uint8_t c = 0;
	const char *display_formats[4] = { "Pan and Scan or Letterbox", "Pan and Scan", "Letterbox", "" };

	printf("{\n");

	// DVD
	printf(" \"dvd\": {");
	printf(" \"title\": \"%s\",", dvd_info.title);
	printf(" \"side\": %u,", dvd_info.side);
	printf(" \"tracks\": %u,", dvd_info.tracks);
	printf(" \"longest track\": %u,", dvd_info.longest_track);
	if(strlen(dvd_info.provider_id))
		printf(" \"provider id\": \"%s\",", dvd_info.provider_id);
	if(strlen(dvd_info.vmg_id))
		printf(" \"vmg id\": \"%s\",", dvd_info.vmg_id);
	printf(" \"video title sets\": %u,", dvd_info.video_title_sets);
	printf(" \"dvdread id\": \"%s\"", dvd_info.dvdread_id);
	printf(" },\n");

	// DVD title tracks
	printf(" \"tracks\": [\n");

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		dvd_track = dvd_tracks[track_number - 1];

		printf("  {\n");
		printf("   \"track\": %u,", dvd_track.track);

		dvd_video = dvd_tracks[track_number - 1].dvd_video;

		printf(" \"length\": \"%s\",", dvd_track.length);
		printf(" \"msecs\": %u,", dvd_track.msecs);
		printf(" \"vts\": %u,", dvd_track.vts);
		printf(" \"ttn\": %u,", dvd_track.ttn);
		printf(" \"valid\": %s\n", dvd_track.valid ? "yes" : "no");

		printf(" \"video\": {");

		printf(" \"codec\": \"%s\",", dvd_video.codec);
		printf(" \"format\": \"%s\",", dvd_video.format);
		printf(" \"aspect ratio\": \"%s\",", dvd_video.aspect_ratio);
		printf(" \"width\": %u,", dvd_video.width);
		printf(" \"height\": %u,", dvd_video.height);
		printf(" \"angles\": %u,", dvd_video.angles);
		printf(" \"fps\": \"%s\",", dvd_video.fps);
		printf(" \"display format\": \"%s\"", display_formats[dvd_video.df]);

		printf(" }");
		if(dvd_track.audio_tracks || dvd_track.subtitles || dvd_track.chapters || dvd_track.cells)
			printf(",");

		// Audio tracks
		if(dvd_track.audio_tracks) {

			printf("\n   \"audio\": [");

			for(c = 0; c < dvd_track.audio_tracks; c++) {

				dvd_audio = dvd_track.dvd_audio_tracks[c];

				printf(" {");
				printf(" \"track\": %u,", dvd_audio.track);
				printf(" \"active\": %u,", dvd_audio.active);
				if(strlen(dvd_audio.lang_code) == DVD_AUDIO_LANG_CODE)
					printf(" \"lang code\": \"%s\",", dvd_audio.lang_code);
				printf(" \"codec\": \"%s\",", dvd_audio.codec);
				printf(" \"channels\": %u,", dvd_audio.channels);
				printf(" \"stream id\": \"%s\"", dvd_audio.stream_id);
				printf(" }");

				if(c + 1 < dvd_track.audio_tracks)
					printf(",");

			}

			printf(" ]");
			if(dvd_track.subtitles || dvd_track.chapters || dvd_track.cells)
				printf(",");
			printf("\n");

		}

		// Subtitles

		if(dvd_track.subtitles) {

			printf("  \"subtitles\": [\n");

			for(c = 0; c < dvd_track.subtitles; c++) {

				dvd_subtitle = dvd_track.dvd_subtitles[c];

				printf("    {\n");

				printf("\"track\": %u,", dvd_subtitle.track);
				printf(" \"active\": %u,", dvd_subtitle.active);

				if(strlen(dvd_subtitle.lang_code) == DVD_SUBTITLE_LANG_CODE)
					printf(" \"lang code\": \"%s\",", dvd_subtitle.lang_code);
				printf(" \"stream id\": \"%s\"", dvd_subtitle.stream_id);
				printf(" }");

				if(c + 1 < dvd_track.subtitles)
					printf(",");
				printf("\n");

			}

			printf("   ]");
			if(dvd_track.chapters || dvd_track.cells)
				printf(",");
			printf("\n");

		}

		// Chapters

		if(dvd_track.chapters) {

			printf("   \"chapters\": [");

			for(c = 0; c < dvd_track.chapters; c++) {

				dvd_chapter = dvd_track.dvd_chapters[c];

				printf(" {");
				printf(" \"chapter\": %u,", dvd_chapter.chapter);
				printf(" \"length\": \"%s\",", dvd_chapter.length);
				printf(" \"msecs\": %u,", dvd_chapter.msecs);
				printf(" \"first cell\": %u,", dvd_chapter.first_cell);
				printf(" \"last cell\": %u", dvd_chapter.last_cell);
				printf(" }");

				if(c + 1 < dvd_track.chapters)
					printf(",");

			}

			printf(" ],\n");

		}

		// Cells

		if(dvd_track.cells) {

			printf("   \"cells\": [");

			for(c = 0; c < dvd_track.cells; c++) {

				dvd_cell = dvd_track.dvd_cells[c];

				printf(" {");
				printf(" \"cell\": %u,", dvd_cell.cell);
				printf(" \"length\": \"%s\",", dvd_cell.length);
				printf(" \"msecs\": %u,", dvd_cell.msecs);
				printf(" \"first sector\": %u,", dvd_cell.first_sector);
				printf(" \"last sector\": %u", dvd_cell.last_sector);
				printf(" }");

				if(c  + 1 < dvd_track.cells)
					printf(",");

			}

			printf(" ]");

			printf("\n");

		}

		printf("  }");

		if(track_number < d_last_track)
			printf(",");

		printf("\n");

	}

	printf(" ]\n");
	printf("}\n");

}
