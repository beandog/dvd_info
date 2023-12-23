#include "dvd_init.h"

/**
 * dvd_tracks index is one-base indexed, with 0 being a reference track
 * audio, subtitles and cells are all zero-base indexed
 */

struct dvd_track dvd_track_init(dvd_reader_t *dvdread_dvd, ifo_handle_t *vmg_ifo, uint16_t track_number, bool init_audio, bool init_subtitles, bool init_chapters, bool init_cells) {

	struct dvd_track dvd_track;
	struct dvd_video dvd_video;
	ifo_handle_t *vts_ifo = NULL;
	uint8_t ix;

	dvd_track.track = track_number;
	dvd_track.dvd_audio_tracks = NULL;
	dvd_track.dvd_subtitles = NULL;
	dvd_track.dvd_chapters = NULL;
	dvd_track.dvd_cells = NULL;

	// Initialize track to default values
	dvd_track.valid = true;
	strncpy(dvd_track.length, "00:00:00.000", 13);
	dvd_track.msecs = 0;
	dvd_track.chapters = 0;
	dvd_track.audio_tracks = 0;
	dvd_track.active_audio_streams = 0;
	dvd_track.subtitles = 0;
	dvd_track.active_subs = 0;
	dvd_track.cells = 0;
	dvd_track.blocks = 0;
	dvd_track.filesize = 0;
	dvd_track.filesize_mbs = 0;

	memset(dvd_video.codec, '\0', sizeof(dvd_video.codec));
	memset(dvd_video.format, '\0', sizeof(dvd_video.format));
	memset(dvd_video.aspect_ratio, '\0', sizeof(dvd_video.aspect_ratio));
	memset(dvd_video.fps, '\0', sizeof(dvd_video.fps));
	dvd_video.width = 0;
	dvd_video.height = 0;
	dvd_video.letterbox = false;
	dvd_video.pan_and_scan = false;
	dvd_video.df = 3;
	dvd_video.angles = 0;

	dvd_track.dvd_video = dvd_video;

	// There are two ways a track can be marked as invalid - either the VTS
	// is bad, or the track has an empty length. The first one, it could be
	// a number of things, but the second is likely by design in order to
	// break DVD software. Invalid DVD tracks appear as completely empty in
	// dvd_info's output.

	// If the IFO is invalid, skip the residing track
	// These are hard to find, so an example DVD is '4b4d78c077ea78576a7de09aee7715d4'
	dvd_track.vts = dvd_vts_ifo_number(vmg_ifo, track_number);

	vts_ifo = ifoOpen(dvdread_dvd, dvd_track.vts);

	if(vts_ifo == NULL) {
		dvd_track.valid = false;
		return dvd_track;
	}

	// If the length is empty, disregard all other data attached to it, and mark as invalid
	dvd_track.msecs = dvd_track_msecs(vmg_ifo, vts_ifo, dvd_track.track);

	// For reference, I've toyed with the idea of marking anything less than 1 second as
	// invalid, so that they are skipped when passing '--valid' option. However, playing
	// one may simply jump to another track, and that doesn't really justify ignoring
	// them. Plus someone can pass '--seconds 1' and it will filter them out anyway.
	if(dvd_track.msecs == 0) {
		dvd_track.valid = false;
		return dvd_track;
	}

	dvd_track.ttn = dvd_track_ttn(vmg_ifo, dvd_track.track);
	dvd_track.ptts = dvd_track_title_parts(vmg_ifo, dvd_track.track);

	dvd_track_length(dvd_track.length, vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.chapters = dvd_track_chapters(vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.blocks = dvd_track_blocks(vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.filesize = dvd_track_filesize(vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.filesize_mbs = dvd_track_filesize_mbs(vmg_ifo, vts_ifo, dvd_track.track);

	dvd_video_codec(dvd_video.codec, vts_ifo);
	dvd_track_video_format(dvd_video.format, vts_ifo);
	dvd_video.width = dvd_video_width(vts_ifo);
	dvd_video.height = dvd_video_height(vts_ifo);
	dvd_video_aspect_ratio(dvd_video.aspect_ratio, vts_ifo);
	dvd_video.letterbox = dvd_video_letterbox(vts_ifo);
	dvd_video.pan_and_scan = dvd_video_pan_scan(vts_ifo);
	dvd_video.df = dvd_video_df(vts_ifo);
	dvd_video.angles = dvd_video_angles(vmg_ifo, dvd_track.track);
	dvd_track_str_fps(dvd_video.fps, vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.dvd_video = dvd_video;

	dvd_track.audio_tracks = dvd_track_audio_tracks(vts_ifo);
	dvd_track.active_audio_streams = dvd_audio_active_tracks(vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.subtitles = dvd_track_subtitles(vts_ifo);
	dvd_track.active_subs = dvd_track_active_subtitles(vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.cells = dvd_track_cells(vmg_ifo, vts_ifo, dvd_track.track);
	dvd_track.filesize_mbs = dvd_track_filesize_mbs(vmg_ifo, vts_ifo, dvd_track.track);

	/** Audio tracks **/
	// FIXME some dvd_audio_ functions are one-indexed, and others are zero
	if(init_audio && dvd_track.audio_tracks > 0) {

		struct dvd_audio dvd_audio;

		dvd_track.dvd_audio_tracks = calloc(dvd_track.audio_tracks, sizeof(struct dvd_audio));

		for(ix = 0; ix < dvd_track.audio_tracks; ix++) {

			dvd_audio.track = ix + 1;

			dvd_audio.active = dvd_audio_active(vmg_ifo, vts_ifo, dvd_track.track, ix);

			dvd_audio.channels = dvd_audio_channels(vts_ifo, ix);

			memset(dvd_audio.stream_id, '\0', sizeof(dvd_audio.stream_id));
			dvd_audio_stream_id(dvd_audio.stream_id, vts_ifo, ix);

			memset(dvd_audio.lang_code, '\0', sizeof(dvd_audio.lang_code));
			dvd_audio_lang_code(dvd_audio.lang_code, vts_ifo, ix);

			memset(dvd_audio.codec, '\0', sizeof(dvd_audio.codec));
			dvd_audio_codec(dvd_audio.codec, vts_ifo, ix);

			dvd_track.dvd_audio_tracks[ix] = dvd_audio;

		}

	}

	/** Subtitles **/
	if(init_subtitles && dvd_track.subtitles > 0) {

		struct dvd_subtitle dvd_subtitle;

		dvd_track.dvd_subtitles = calloc(dvd_track.subtitles, sizeof(struct dvd_subtitle));

		for(ix = 0; ix < dvd_track.subtitles; ix++) {

			dvd_subtitle.track = ix + 1;

			// dvd_subtitle_ functions are one-indexed
			dvd_subtitle.active = dvd_subtitle_active(vmg_ifo, vts_ifo, dvd_track.track, dvd_subtitle.track);

			memset(dvd_subtitle.stream_id, 0, sizeof(dvd_subtitle.stream_id));
			dvd_subtitle_stream_id(dvd_subtitle.stream_id, ix);

			memset(dvd_subtitle.lang_code, 0, sizeof(dvd_subtitle.lang_code));
			dvd_subtitle_lang_code(dvd_subtitle.lang_code, vts_ifo, ix);

			dvd_track.dvd_subtitles[ix] = dvd_subtitle;

		}

	}

	/** Chapters **/
	if(init_chapters) {

		struct dvd_chapter dvd_chapter;

		dvd_track.dvd_chapters = calloc(dvd_track.chapters, sizeof(struct dvd_chapter));

		for(ix = 0; ix < dvd_track.chapters; ix++) {

			dvd_chapter.chapter = ix + 1;

			// dvd_chapter_ functions are one-indexed
			dvd_chapter_length(dvd_chapter.length, vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
			dvd_chapter.msecs = dvd_chapter_msecs(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
			dvd_chapter.first_cell = dvd_chapter_first_cell(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
			dvd_chapter.last_cell = dvd_chapter_last_cell(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
			dvd_chapter.blocks = dvd_chapter_blocks(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
			dvd_chapter.filesize = dvd_chapter_filesize(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);
			dvd_chapter.filesize_mbs = dvd_chapter_filesize_mbs(vmg_ifo, vts_ifo, dvd_track.track, dvd_chapter.chapter);

			dvd_track.dvd_chapters[ix] = dvd_chapter;

		}

	}

	/** Cells **/
	if(init_cells) {

		struct dvd_cell dvd_cell;

		dvd_track.dvd_cells = calloc(dvd_track.cells, sizeof(struct dvd_cell));

		for(ix = 0; ix < dvd_track.cells; ix++) {

			dvd_cell.cell = ix + 1;

			// dvd_cell_ functions are one-indexed
			dvd_cell_length(dvd_cell.length, vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);

			dvd_cell.msecs = dvd_cell_msecs(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.first_sector = dvd_cell_first_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.last_sector = dvd_cell_last_sector(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.blocks = dvd_cell_blocks(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.filesize = dvd_cell_filesize(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);
			dvd_cell.filesize_mbs = dvd_cell_filesize_mbs(vmg_ifo, vts_ifo, dvd_track.track, dvd_cell.cell);

			dvd_track.dvd_cells[ix] = dvd_cell;

		}

	}

	return dvd_track;

}

struct dvd_track *dvd_tracks_init(dvd_reader_t *dvdread_dvd, ifo_handle_t *vmg_ifo, bool init_audio, bool init_subtitles, bool init_chapters, bool init_cells) {

	uint16_t track_number = 1;
	uint16_t longest_track = 1;
	uint32_t longest_msecs = 0;
	uint16_t num_tracks = dvd_tracks(vmg_ifo);
	struct dvd_track *tracks = calloc(num_tracks + 1, sizeof(struct dvd_track));

	for(track_number = 1; track_number < num_tracks + 1; track_number++) {
		tracks[track_number] = dvd_track_init(dvdread_dvd, vmg_ifo, track_number, init_audio, init_subtitles, init_chapters, init_cells);

		if(tracks[track_number].msecs > longest_msecs) {
			longest_track = track_number;
			longest_msecs = tracks[track_number].msecs;
		}

	}

	// Cheat and use track 0 as a reference to the longest track
	tracks[0].track = longest_track;

	return tracks;

}
