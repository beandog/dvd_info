#ifndef DVD_INIT_H
#define DVD_INIT_H

#include <stdlib.h>
#include "dvd_specs.h"
#include "dvd_audio.h"
#include "dvd_cell.h"
#include "dvd_chapter.h"
#include "dvd_info.h"
#include "dvd_subtitles.h"
#include "dvd_time.h"
#include "dvd_track.h"
#include "dvd_video.h"
#include "dvd_vmg_ifo.h"
#include "dvd_vts.h"

struct dvd_track dvd_track_init(dvd_reader_t *dvdread_dvd, ifo_handle_t *vmg_ifo, uint16_t track_number, bool init_audio, bool init_subtitles, bool init_chapters, bool init_cells);

struct dvd_track *dvd_tracks_init(dvd_reader_t *dvdread_dvd, ifo_handle_t *vmg_ifo, bool init_audio, bool init_subtitles, bool init_chapters, bool init_cells);

#endif
