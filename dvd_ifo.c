#include "dvd_ifo.h"

bool vmgm_audio_streams_ok(unsigned int i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool mpeg_version_ok(unsigned int i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool video_format_ok(unsigned int i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool display_aspect_ratio_ok(unsigned int i) {

	if((i != 0) && (i != 3))
		return false;
	else
		return true;

}

bool permitted_df_ok(unsigned int i) {

	if(i > 3)
		return false;
	else
		return true;

}

bool line21_cc_ok(unsigned int video_format, unsigned int line21_cc) {

	if(video_format != 0 && line21_cc > 0)
		return false;
	else
		return true;

}

bool video_picture_size_ok(unsigned int i) {

	if(i > 3)
		return false;
	else
		return true;

}

bool letterboxed_ok(unsigned int i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool film_mode_ok(unsigned int i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool audio_format_ok(unsigned int i) {

	if(i == 1 || i == 5 || i > 6)
		return false;
	else
		return true;

}

bool quantization_ok(unsigned int i) {

	if(i > 3)
		return false;
	else
		return true;

}

bool sample_frequency_ok(unsigned int i) {

	if(i > 1)
		return false;
	else
		return true;

}

bool channels_ok(unsigned int i) {

	if(i > 5)
		return false;
	else
		return true;

}
