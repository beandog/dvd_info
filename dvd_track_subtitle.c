#include "dvd_track_subtitle.h"

// TODO this function could stand a lot of strict error checking to see what the subtitle status is, and offer different return codes
// Have dvd_debug check for issues here.
int dvd_track_subtitle_lang_code(const ifo_handle_t *vts_ifo, const int subtitle_track, char *p) {

	char lang_code[3] = {'\0'};
	subp_attr_t *subp_attr;

	subp_attr = &vts_ifo->vtsi_mat->vts_subp_attr[subtitle_track];

	// Same check as ifo_print
	if(subp_attr->type == 0 && subp_attr->lang_code == 0 && subp_attr->zero1 == 0 && subp_attr->zero2 == 0 && subp_attr->lang_extension == 0) {
		strncpy(p, "xx", 3);
		return 1;
	}

	snprintf(lang_code, 3, "%c%c", subp_attr->lang_code >> 8, subp_attr->lang_code & 0xff);
	// Following the same logic as lsdvd -- if the first char is invalid(?)
	// then set it to 'xx' as well.
	if(!lang_code[0]) {
		strncpy(p, "xx", 3);
		return 2;
	}

	strncpy(p, lang_code, 3);

	return 0;

}

int dvd_track_subtitle_stream_id(const int subtitle_track) {

	return 0x20 + subtitle_track;

}
