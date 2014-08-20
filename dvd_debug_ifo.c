#include "dvd_debug_ifo.h"

bool vmgm_audio_streams_ok(uint32_t i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool mpeg_version_ok(uint32_t i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool video_format_ok(uint32_t i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool display_aspect_ratio_ok(uint32_t i) {

	if(i != 0 && i != 3)
		return false;
	else
		return true;

}

bool permitted_df_ok(uint32_t i) {

	if(i > 3)
		return false;
	else
		return true;

}

bool line21_cc_ok(uint32_t video_format, uint32_t line21_cc) {

	if(video_format != 0 && line21_cc > 0)
		return false;
	else
		return true;

}

bool video_picture_size_ok(uint32_t i) {

	if(i > 3)
		return false;
	else
		return true;

}

bool letterboxed_ok(uint32_t i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool film_mode_ok(uint32_t i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool audio_format_ok(uint32_t i) {

	if(i == 1 || i == 5 || i > 6)
		return false;
	else
		return true;

}

bool quantization_ok(uint32_t i) {

	if(i > 3)
		return false;
	else
		return true;

}

bool sample_frequency_ok(uint32_t i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool channels_ok(uint32_t i) {

	if(i > 5)
		return false;
	else
		return true;

}
