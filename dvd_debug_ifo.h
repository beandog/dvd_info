#include <stdio.h>
#include <stdbool.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_types.h>

bool vmgm_audio_streams_ok(uint32_t i);
bool mpeg_version_ok(uint32_t i);
bool video_format_ok(uint32_t i);
bool display_aspect_ratio_ok(uint32_t i);
bool permitted_df_ok(uint32_t i);
bool line21_cc_ok(uint32_t video_format, uint32_t line21_cc);
bool video_picture_size_ok(uint32_t i);
bool letterboxed_ok(uint32_t i);
bool film_mode_ok(uint32_t i);
bool audio_format_ok(uint32_t i);
bool quantization_ok(uint32_t i);
bool sample_frequency_ok(uint32_t i);
bool channels_ok(uint32_t i);
