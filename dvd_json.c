#include "dvd_json.h"

void dvd_json(struct dvd_info dvd_info, struct dvd_track dvd_tracks[], uint16_t track_number, uint16_t d_first_track, uint16_t d_last_track) {

	struct dvd_track dvd_track;
	struct dvd_video dvd_video;
	struct dvd_audio dvd_audio;
	struct dvd_subtitle dvd_subtitle;
	struct dvd_chapter dvd_chapter;
	struct dvd_cell dvd_cell;
	uint8_t c = 0;

	printf("{\n");

	// DVD
	printf(" \"dvd\": {\n");
	printf("  \"title\": \"%s\",\n", dvd_info.title);
	printf("  \"side\": %u,\n", dvd_info.side);
	printf("  \"tracks\": %u,\n", dvd_info.tracks);
	printf("  \"longest track\": %u,\n", dvd_info.longest_track);
	if(strlen(dvd_info.provider_id))
		printf("  \"provider id\": \"%s\",\n", dvd_info.provider_id);
	if(strlen(dvd_info.vmg_id))
		printf("  \"vmg id\": \"%s\",\n", dvd_info.vmg_id);
	printf("  \"video title sets\": %u,\n", dvd_info.video_title_sets);
	printf("  \"dvdread id\": \"%s\"\n", dvd_info.dvdread_id);
	printf(" },\n");

	// DVD title tracks
	printf(" \"tracks\": [\n");

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		dvd_track = dvd_tracks[track_number - 1];

		printf("  {\n");
		printf("   \"track\": %u,\n", dvd_track.track);

		// If the title track is invalid, skip to the next one
		printf("   \"valid\": %u", dvd_track.valid);
		if(dvd_track.valid == 0) {
			printf("\n  }");
			if(track_number + 1 <= d_last_track)
				printf(",");
			printf("\n");
			continue;
		}

		printf(",\n");

		dvd_video = dvd_tracks[track_number - 1].dvd_video;

		printf("   \"length\": \"%s\",\n", dvd_track.length);
		printf("   \"msecs\": %u,\n", dvd_track.msecs);
		printf("   \"vts\": %u,\n", dvd_track.vts);
		printf("   \"ttn\": %u,\n", dvd_track.ttn);

		printf("   \"video\": {\n");

		if(strlen(dvd_video.codec))
			printf("    \"codec\": \"%s\",\n", dvd_video.codec);
		if(strlen(dvd_video.format))
			printf("    \"format\": \"%s\",\n", dvd_video.format);
		if(strlen(dvd_video.aspect_ratio))
			printf("    \"aspect ratio\": \"%s\",\n", dvd_video.aspect_ratio);

		// FIXME needs cleanup
		/*
		if(dvd_video.df == 0)
			printf("   \"df\": \"Pan and Scan + Letterbox\",\n");
		else if(dvd_video.df == 1)
			printf("   \"df\": \"Pan and Scan\",\n");
		else if(dvd_video.df == 2)
			printf("   \"df\": \"Letterbox\",\n");
		*/

		printf("    \"width\": %u,\n", dvd_video.width);
		printf("    \"height\": %u,\n", dvd_video.height);
		printf("    \"angles\": %u", dvd_video.angles);

		// Only display FPS if it's been populated as a string
		if(strlen(dvd_video.fps))
			printf(",\n    \"fps\": \"%s\"\n", dvd_video.fps);
		printf("   },\n");

		// Audio tracks
		if(dvd_track.audio_tracks) {

			printf("   \"audio\": [\n");

			for(c = 0; c < dvd_track.audio_tracks; c++) {

				dvd_audio = dvd_track.dvd_audio_tracks[c];

				printf("    {\n");
				printf("     \"track\": %u,\n", dvd_audio.track);
				printf("     \"active\": %u,\n", dvd_audio.active);
				if(strlen(dvd_audio.lang_code) == DVD_AUDIO_LANG_CODE)
					printf("     \"lang code\": \"%s\",\n", dvd_audio.lang_code);
				printf("     \"codec\": \"%s\",\n", dvd_audio.codec);
				printf("     \"channels\": %u,\n", dvd_audio.channels);
				printf("     \"stream id\": \"%s\"\n", dvd_audio.stream_id);
				printf("    }");

				if(c + 1 < dvd_track.audio_tracks)
					printf(",");
				printf("\n");

			}

			printf("   ],\n");

		}

		// Subtitles

		if(dvd_track.subtitles) {

			printf("   \"subtitles\": [\n");

			for(c = 0; c < dvd_track.subtitles; c++) {

				dvd_subtitle = dvd_track.dvd_subtitles[c];

				printf("    {\n");

				printf("     \"track\": %u,\n", dvd_subtitle.track);
				printf("     \"active\": %u,\n", dvd_subtitle.active);

				if(strlen(dvd_subtitle.lang_code) == DVD_SUBTITLE_LANG_CODE)
					printf("     \"lang code\": \"%s\",\n", dvd_subtitle.lang_code);
				printf("     \"stream id\": \"%s\"\n", dvd_subtitle.stream_id);
				printf("    }");

				if(c + 1 < dvd_track.subtitles)
					printf(",");
				printf("\n");

			}

			printf("   ],\n");

		}

		// Chapters

		if(dvd_track.chapters) {

			printf("   \"chapters\": [\n");

			for(c = 0; c < dvd_track.chapters; c++) {

				dvd_chapter = dvd_track.dvd_chapters[c];

				printf("    {\n");
				printf("     \"chapter\": %u,\n", dvd_chapter.chapter);
				printf("     \"length\": \"%s\",\n", dvd_chapter.length);
				printf("     \"msecs\": %u,\n", dvd_chapter.msecs);
				printf("     \"startcell\": %u\n", dvd_chapter.startcell);
				printf("    }");

				if(c + 1 < dvd_track.chapters)
					printf(",");
				printf("\n");

			}

			printf("   ],\n");

		}

		// Cells

		if(dvd_track.cells) {

			printf("   \"cells\": [\n");

			for(c = 0; c < dvd_track.cells; c++) {

				dvd_cell = dvd_track.dvd_cells[c];

				printf("    {\n");
				printf("     \"cell\": %u,\n", dvd_cell.cell);
				printf("     \"length\": \"%s\",\n", dvd_cell.length);
				printf("     \"msecs\": %u\n", dvd_cell.msecs);
				printf("     \"first sector\": %u\n", dvd_cell.first_sector);
				printf("     \"last sector\": %u\n", dvd_cell.first_sector);
				printf("    }");

				if(c  + 1 < dvd_track.cells)
					printf(",");
				printf("\n");

			}

			printf("   ]\n");
			printf("  }");

			if(track_number + 1 <= d_last_track)
				printf(",");
			printf("\n");

		}

	}

	printf(" ]\n");
	printf("}\n");

}
