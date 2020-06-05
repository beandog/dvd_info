#ifndef DVD_INFO_PLAYER_H
#define DVD_INFO_PLAYER_H

struct dvd_player {
	char config_dir[PATH_MAX];
	char mpv_config_dir[PATH_MAX];
};

struct dvd_playback {
	uint16_t track;
	uint8_t first_chapter;
	uint8_t last_chapter;
	bool fullscreen;
	bool detelecine;
	char audio_lang[3];
	char audio_stream_id[4];
	bool subtitles;
	char subtitles_lang[3];
	char subtitles_stream_id[4];
	char mpv_chapters_range[32];
};

#endif
