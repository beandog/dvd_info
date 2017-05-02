#ifndef _DVD_INFO_H_
#define _DVD_INFO_H_

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/dvd_udf.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>

// Default DVD device
#ifdef __linux__
#include <linux/cdrom.h>
#define DEFAULT_DVD_DEVICE "/dev/sr0"
#elif __FreeBSD__ || __DragonFly__
#define DEFAULT_DVD_DEVICE "/dev/acd0"
#elif __NetBSD__
#define DEFAULT_DVD_DEVICE "/dev/cd0d"
#elif __OpenBSD__
#define DEFAULT_DVD_DEVICE "/dev/cd0c"
#elif __APPLE__ && __MACH__
#define DEFAULT_DVD_DEVICE "/dev/disk1"
#else
#define DEFAULT_DVD_DEVICE "/dev/dvd"
#endif

// String lengths for DVD metadata
#define DVD_TITLE 32
#define DVD_PROVIDER_ID 32
#define DVD_VMG_ID 12
#define DVD_SPECIFICATION_VERSION 3
#define DVD_DVDREAD_ID 32

#define DVD_VTS_ID 12

#define DVD_TRACK_LENGTH 12

#define DVD_VIDEO_CODEC 5
#define DVD_VIDEO_FORMAT 4
#define DVD_VIDEO_ASPECT_RATIO 4
#define DVD_VIDEO_FPS 5

#define DVD_AUDIO_STREAM_ID 4
#define DVD_AUDIO_LANG_CODE 2
#define DVD_AUDIO_CODEC 4
#define DVD_AUDIO_STREAM_LIMIT 8

#define DVD_SUBTITLE_STREAM_ID 4
#define DVD_SUBTITLE_LANG_CODE 2
#define DVD_SUBTITLE_STREAM_LIMIT 32

#define DVD_CHAPTER_LENGTH 12

#define DVD_CELL_LENGTH 12

void print_usage(char *binary);

struct dvd_info {
	char dvdread_id[DVD_DVDREAD_ID + 1];
	uint16_t video_title_sets;
	uint8_t side;
	char title[DVD_TITLE + 1];
	char provider_id[DVD_PROVIDER_ID + 1];
	char vmg_id[DVD_VMG_ID + 1];
	uint16_t tracks;
	uint16_t longest_track;
};

struct dvd_vts {
	uint16_t vts;
	char id[DVD_VTS_ID + 1];
	ssize_t blocks;
	ssize_t filesize;
	int vobs;
};

struct dvd_vob {
	uint16_t vts;
	uint16_t vob;
	ssize_t blocks;
	ssize_t filesize;
};

struct dvd_video {
	char codec[DVD_VIDEO_CODEC + 1];
	char format[DVD_VIDEO_FORMAT + 1];
	char aspect_ratio[DVD_VIDEO_ASPECT_RATIO + 1];
	uint16_t width;
	uint16_t height;
	bool letterbox;
	bool pan_and_scan;
	uint8_t df;
	char fps[DVD_VIDEO_FPS + 1];
	uint8_t angles;
};

struct dvd_audio {
	uint8_t track;
	uint8_t active;
	char stream_id[DVD_AUDIO_STREAM_ID + 1];
	char lang_code[DVD_AUDIO_LANG_CODE + 1];
	char codec[DVD_AUDIO_CODEC + 1];
	uint8_t channels;
};

struct dvd_subtitle {
	uint8_t track;
	uint8_t active;
	char stream_id[DVD_SUBTITLE_STREAM_ID + 1];
	char lang_code[DVD_SUBTITLE_LANG_CODE + 1];
};

struct dvd_chapter {
	uint8_t chapter;
	char length[DVD_CHAPTER_LENGTH + 1];
	uint32_t msecs;
	uint8_t startcell;
};

struct dvd_cell {
	uint8_t cell;
	char length[DVD_CELL_LENGTH + 1];
	uint32_t msecs;
	uint32_t first_sector;
	uint32_t last_sector;
	ssize_t blocks;
	ssize_t filesize;
};

struct dvd_track {
	uint16_t track;
	uint8_t valid;
	uint16_t vts;
	uint8_t ttn;
	char length[DVD_TRACK_LENGTH + 1];
	uint32_t msecs;
	uint8_t chapters;
	uint8_t audio_tracks;
	uint8_t subtitles;
	uint8_t cells;
	struct dvd_video dvd_video;
	struct dvd_audio *dvd_audio_tracks;
	uint8_t active_audio;
	struct dvd_subtitle *dvd_subtitles;
	uint8_t active_subs;
	struct dvd_chapter *dvd_chapters;
	struct dvd_cell *dvd_cells;
	ssize_t blocks;
	ssize_t filesize;
};

int main(int argc, char **argv);

#endif
