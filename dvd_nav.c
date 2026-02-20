#include "dvd_nav.h"

/**
 * Use libdvnav to get the DVD region.
 *
 * Possible regions are 0 (region-free) or 1 through 8
 * See https://en.wikipedia.org/wiki/DVD_region_code for region codes
 *
 * If the value is out-of that range, return 0.
 */
int32_t dvd_info_region(dvdnav_t *dvdnav_dvd) {

	int32_t dvdnav_region;
	int32_t dvdnav_region_mask;

	dvdnav_region = dvdnav_get_disk_region_mask(dvdnav_dvd, &dvdnav_region_mask);

	if(dvdnav_region_mask < 0 || dvdnav_region_mask > 8)
		return 0;

	return dvdnav_region_mask;

}
