#include "dvd_time.h"

/**
 * Convert any value of dvd_time to milliseconds -- uses the same code set as
 * lsdvd for compatability.
 *
 * Regarding the bit-shifting in the function, I've broken down the
 * calculations to separate individual time metrics to make it a bit easier to
 * read, though I admittedly don't know enough about bit shifting to know what
 * it's doing or why it doesn't just do simple calculation.  It's worth seeing
 * ifo_print_time() from libdvdread as a comparative reference as well.
 *
 * Old notes:
 *
 * Though it is not implemented here, I have a theory that msecs should be a
 * floating value to a decimal point of 2, and that the framerate should be
 * a whole integer (30) instead of a floating point decimal (29.97).  The
 * rest of these notes explain that approach, thoough it is not used.  They
 * are kept here for historical purposes for my own benefit.
 *
 * Originally, the title track length was calculated by looking at the PGC for
 * the track itself.  With that approach, the total length for the track did
 * not match the sum of the total length of all the individual cells.  The
 * offset was small, in the range of -5 to +5 *milliseconds* and it did not
 * occur all the time.  To fix the title track length, I changed it to use the
 * total from all the cells.  That by itself changed the track lengths to msec
 * offsets that looked a little strange from their original parts.  Fex, 500
 * would change to 501, 900 to 898, etc.  Since it's far more likely that the
 * correct value was a whole integer (.500 seconds happens all the time), then
 * it needed to be changed.
 *
 * According to http://stnsoft.com/DVD/pgc.html it looks like the FPS values are
 * integers, either 25 or 30.  So this uses 3000 instead of 2997 for that reason.
 * as well.  With these two changes only does minor changes, and they are always
 * something like 734 to 733, 667 to 666, 834 to 833, and so on.
 *
 * Either way, it's probably safe to say that calculating the exact length of a
 * cell or track is hard, and that this is the best approxmiation that
 * preserves what the original track length is estimated to be from the PGC. So
 * this method is used as a preference, even though I don't necessarily want to
 * recommend relying on either approach for complete accuracy.
 *
 * Making these changes though is going to differ in displaying the track
 * length from other DVD applications -- again, in milliseconds only -- but I
 * justify this approach in the sense that using this way, the cell, chapter
 * and title track lengths all match up.
 *
 * @param dvd_time dvd_time
 */
uint32_t dvd_time_to_milliseconds(dvd_time_t *dvd_time) {

	int i = (dvd_time->frame_u & 0xc0) >> 6;

	if(i < 0 || i > 3)
		return 0;

	uint32_t framerates[4] = {0, 2500, 0, 2997};
	uint32_t framerate = framerates[i];

	uint32_t hours = (((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f));
	uint32_t minutes = (((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f));
	uint32_t seconds = (((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f));
	uint32_t msecs = 0;
	if(framerate > 0)
		msecs = ((((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000) / framerate;

	uint32_t total = (hours * 3600000);
	total += (minutes * 60000);
	total += (seconds * 1000);
	total += msecs;

	return total;

	// For reference, here is how lsdvd's dvdtime2msec function works
	/*
	double framerates[4] = {-1.0, 25.00, -1.0, 29.97};
	int i = (dvd_time->frame_u & 0xc0) >> 6;
	double framerate = framerates[i];
	uint32_t msecs = 0;

	msecs = (((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f)) * 3600000;
	msecs += (((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f)) * 60000;
	msecs += (((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f)) * 1000;
	if(framerate > 0)
		msecs += (((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 1000.0 / framerate;

	return msecs;
	*/

}

/**
 * Convert milliseconds to format hh:mm:ss.ms
 *
 * @param milliseconds milliseconds
 */
void milliseconds_length_format(char *dest_str, const uint32_t milliseconds) {

	uint32_t total_seconds = milliseconds / 1000;
	uint32_t hours = total_seconds / (3600);
	uint32_t minutes = (total_seconds / 60) % 60;
	if(minutes > 59)
		minutes -= 59;
	uint32_t seconds = total_seconds % 60;
	if(seconds > 59)
		seconds -= 59;
	uint32_t msecs = milliseconds - (hours * 3600 * 1000) - (minutes * 60 * 1000) - (seconds * 1000);

	sprintf(dest_str, "%02u:%02u:%02u.%03u", hours, minutes, seconds, msecs);

}
