#ifndef DVD_INFO_SPECS_H
#define DVD_INFO_SPECS_H

// The max size of a dual-layer disc is 8.5 GB
// or, 1024 * 1024 * 1024 * 8.5
// The value falls into an int64_t range
#define DVD_MAX_BYTES 9126805504

// DVD storage is in blocks, with one being 2048 bytes
// 9126805504 / 2048
// The value falls into an int32_t range
#define DVD_MAX_BLOCKS 4456448

#define DVD_TITLE 32
#define DVD_PROVIDER_ID 32
#define DVD_VMG_ID 12
#define DVD_SPECIFICATION_VERSION 3
#define DVD_DVDREAD_ID 32
#define DVD_VTS_ID 12
#define DVD_MAX_VTS_IFOS 100
#define DVD_MAX_TRACKS 99
#define DVD_TRACK_LENGTH 12
#define DVD_VIDEO_CODEC 5
#define DVD_VIDEO_FORMAT 4
#define DVD_VIDEO_ASPECT_RATIO 4
#define DVD_VIDEO_FPS 5
#define DVD_AUDIO_STREAM_ID 8
#define DVD_AUDIO_LANG_CODE 2
#define DVD_AUDIO_CODEC 4
#define DVD_AUDIO_STREAM_LIMIT 8
#define DVD_SUBTITLE_STREAM_ID 8
#define DVD_SUBTITLE_LANG_CODE 2
#define DVD_SUBTITLE_STREAM_LIMIT 32
#define DVD_CHAPTER_LENGTH 12
#define DVD_CELL_LENGTH 12

#endif
