#include <stdio.h>
#include <stdbool.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_types.h>

bool vmgm_audio_streams_ok(unsigned int i);
bool mpeg_version_ok(unsigned int i);
bool video_format_ok(unsigned int i);
bool display_aspect_ratio_ok(unsigned int i);
bool permitted_df_ok(unsigned int i);
bool line21_cc_ok(unsigned int video_format, unsigned int line21_cc);
bool video_picture_size_ok(unsigned int i);
bool letterboxed_ok(unsigned int i);
bool film_mode_ok(unsigned int i);
bool audio_format_ok(unsigned int i);
bool quantization_ok(unsigned int i);
bool sample_frequency_ok(unsigned int i);
bool channels_ok(unsigned int i);
