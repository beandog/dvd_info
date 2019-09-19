#ifndef DVD_INFO_JSON_H
#define DVD_INFO_JSON_H

#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_audio.h"
#include "dvd_subtitles.h"
#include "dvd_chapter.h"
#include "dvd_cell.h"

void dvd_json(struct dvd_info dvd_info, struct dvd_track dvd_tracks[], uint16_t track_number, uint16_t d_first_track, uint16_t d_last_track);

#endif
