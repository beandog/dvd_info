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
	printf(" \"dvd\": {\n");
	printf("   \"title\": \"%s\",\n", dvd_info.title);
	printf("   \"side\": %" PRIu8 ",\n", dvd_info.side);
	printf("   \"tracks\": %" PRIu16 ",\n", dvd_info.tracks);
	printf("   \"longest track\": %u,\n", dvd_info.longest_track);
	if(strlen(dvd_info.provider_id))
		printf("   \"provider id\": \"%s\",\n", dvd_info.provider_id);
	if(strlen(dvd_info.vmg_id))
		printf("   \"vmg id\": \"%s\",\n", dvd_info.vmg_id);
	printf("   \"video title sets\": %" PRIu16 ",\n", dvd_info.video_title_sets);
	printf("   \"dvdread id\": \"%s\"\n", dvd_info.dvdread_id);
	printf(" },\n");

	// DVD title tracks
	printf(" \"tracks\": [\n");

	for(track_number = d_first_track; track_number <= d_last_track; track_number++) {

		dvd_track = dvd_tracks[track_number];

		printf("   {\n");
		printf("     \"track\": %u,\n", dvd_track.track);

		dvd_video = dvd_tracks[track_number].dvd_video;

		printf("     \"length\": \"%s\",\n", dvd_track.length);
		printf("     \"msecs\": %" PRIu32 ",\n", dvd_track.msecs);
		printf("     \"vts\": %" PRIu16 ",\n", dvd_track.vts);
		printf("     \"ttn\": %" PRIu8 ",\n", dvd_track.ttn);
		printf("     \"ptts\": %" PRIu16 ",\n", dvd_track.ptts);
		printf("     \"blocks\": %" PRIu64 ",\n", dvd_track.blocks);
		printf("     \"filesize\": %" PRIu64 ",\n", dvd_track.filesize);
		printf("     \"valid\": \"%s\",\n", dvd_track.valid ? "yes" : "no");

		printf("     \"video\": {\n");
		printf("       \"codec\": \"%s\",\n", dvd_video.codec);
		printf("       \"format\": \"%s\",\n", dvd_video.format);
		printf("       \"aspect ratio\": \"%s\",\n", dvd_video.aspect_ratio);
		printf("       \"width\": %" PRIu16 ",\n", dvd_video.width);
		printf("       \"height\": %" PRIu16 ",\n", dvd_video.height);
		printf("       \"angles\": %" PRIu8 ",\n", dvd_video.angles);
		printf("       \"fps\": \"%s\",\n", dvd_video.fps);
		printf("       \"display format\": \"%s\"\n", display_formats[dvd_video.df]);
		printf("     }");

		if(dvd_track.audio_tracks || dvd_track.subtitles || dvd_track.chapters || dvd_track.cells)
			printf(",");

		// Audio tracks
		if(dvd_track.audio_tracks) {

			printf("\n");
			printf("     \"audio\": [\n");

			for(c = 0; c < dvd_track.audio_tracks; c++) {

				dvd_audio = dvd_track.dvd_audio_tracks[c];

				printf("       {\n");
				printf("         \"track\": %" PRIu16 ",\n", dvd_audio.track);
				printf("         \"active\": %" PRIu16 ",\n", dvd_audio.active);
				if(strlen(dvd_audio.lang_code) == DVD_AUDIO_LANG_CODE)
					printf("         \"lang code\": \"%s\",\n", dvd_audio.lang_code);
				printf("         \"codec\": \"%s\",\n", dvd_audio.codec);
				printf("         \"channels\": %" PRIu8 ",\n", dvd_audio.channels);
				printf("         \"quantization\": \"%s\",\n", dvd_audio.quantization);
				printf("         \"type\": \"%s\",\n", dvd_audio.type);
				printf("         \"stream id\": \"%s\"\n", dvd_audio.stream_id);
				printf("       }");

				if(c + 1 < dvd_track.audio_tracks)
					printf(",");
				printf("\n");

			}

			printf("     ]");
			if(dvd_track.subtitles || dvd_track.chapters || dvd_track.cells)
				printf(",");
			printf("\n");

		}

		// Subtitles

		if(dvd_track.subtitles) {

			printf("     \"subtitles\": [\n");

			for(c = 0; c < dvd_track.subtitles; c++) {

				dvd_subtitle = dvd_track.dvd_subtitles[c];

				printf("       {\n");

				printf("         \"track\": %" PRIu16 ",\n", dvd_subtitle.track);
				printf("         \"active\": %" PRIu16 ",\n", dvd_subtitle.active);

				if(strlen(dvd_subtitle.lang_code) == DVD_SUBTITLE_LANG_CODE)
					printf("         \"lang code\": \"%s\",\n", dvd_subtitle.lang_code);
				printf("         \"stream id\": \"%s\"\n", dvd_subtitle.stream_id);
				printf("       }");

				if(c + 1 < dvd_track.subtitles)
					printf(",");
				printf("\n");

			}

			printf("     ]");
			if(dvd_track.chapters || dvd_track.cells)
				printf(",");
			printf("\n");

		}

		// Chapters

		if(dvd_track.chapters) {

			printf("     \"chapters\": [\n");

			for(c = 0; c < dvd_track.chapters; c++) {

				dvd_chapter = dvd_track.dvd_chapters[c];

				printf("       {\n");
				printf("         \"chapter\": %" PRIu8 ",\n", dvd_chapter.chapter);
				printf("         \"length\": \"%s\",\n", dvd_chapter.length);
				printf("         \"msecs\": %" PRIu32 ",\n", dvd_chapter.msecs);
				printf("         \"first cell\": %" PRIu8 ",\n", dvd_chapter.first_cell);
				printf("         \"last cell\": %" PRIu8 ",\n", dvd_chapter.last_cell);
				printf("         \"blocks\": %" PRIu64 ",\n", dvd_chapter.blocks);
				printf("         \"filesize\": %" PRIu64 "\n", dvd_chapter.filesize);
				printf("       }");

				if(c + 1 < dvd_track.chapters)
					printf(",");
				printf("\n");

			}

			printf("     ],\n");

		}

		// Cells

		if(dvd_track.cells) {

			printf("   \"cells\": [\n");

			for(c = 0; c < dvd_track.cells; c++) {

				dvd_cell = dvd_track.dvd_cells[c];

				printf("       {\n");
				printf("         \"cell\": %" PRIu8 ",\n", dvd_cell.cell);
				printf("         \"length\": \"%s\",\n", dvd_cell.length);
				printf("         \"msecs\": %" PRIu32 ",\n", dvd_cell.msecs);
				printf("         \"first sector\": %" PRIu64 ",\n", dvd_cell.first_sector);
				printf("         \"last sector\": %" PRIu64 ",\n", dvd_cell.last_sector);
				printf("         \"filesize\": %" PRIu64 "\n", dvd_cell.filesize);
				printf("       }");

				if(c  + 1 < dvd_track.cells)
					printf(",");
				printf("\n");

			}

			printf("     ]\n");

		}

		printf("   }");

		if(track_number < d_last_track)
			printf(",");

		printf("\n");

	}

	printf(" ]\n");
	printf("}\n");

}
