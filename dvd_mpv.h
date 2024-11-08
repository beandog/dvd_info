#ifndef DVD_INFO_MPV_H
#define DVD_INFO_MPV_H

struct dvd_player {
	char config_dir[PATH_MAX];
	char mpv_config_dir[PATH_MAX];
};

struct dvd_playback {
	uint16_t track;
	uint8_t first_chapter;
	uint8_t last_chapter;
	bool fullscreen;
	bool deinterlace;
	char audio_lang[3];
	char audio_stream_id[4];
	bool subtitles;
	char subtitles_lang[3];
	char subtitles_stream_id[4];
	char mpv_chapters_range[32];
};

struct dvd_rip {
	uint16_t track;
	uint8_t first_chapter;
	uint8_t last_chapter;
	uint8_t mpv_last_chapter;
	char filename[PATH_MAX];
	char config_dir[PATH_MAX];
	char mpv_config_dir[PATH_MAX];
	char container[5];
	bool encode_video;
	char vcodec[256];
	char vcodec_opts[256];
	char vcodec_log_level[6];
	bool encode_audio;
	char audio_lang[3];
	char audio_stream_id[4];
	char acodec[256];
	char acodec_opts[256];
	uint16_t video_bitrate;
	uint16_t audio_bitrate;
	bool encode_subtitles;
	char subtitles_lang[3];
	char subtitles_stream_id[4];
	char vf_opts[256];
	char of_opts[256];
	uint8_t crf;
};

#endif
