#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include "dvd_device.h"

/**
 * Check to see if the file can actually be accessed by the user running
 * the program.  This would check to see if the file exists as well.
 *
 * @param device_filename filename (/dev/dvd, dvd.iso, etc.)
 * @return bool can access device or not
 */
bool dvd_device_access(const char *device_filename) {

	if(access(device_filename, F_OK) == 0)
		return true;
	else
		return false;

}

/**
 * Use open() to create a file descriptor to the DVD.
 * See also 'man 3 open'
 *
 * @param device_filename device filename (/dev/dvd, dvd.iso, etc.)
 * @return file descriptor, -1 if failed
 */
int dvd_device_open(const char *device_filename) {

	return open(device_filename, O_RDONLY | O_NONBLOCK);

}

/**
 * Use close() to close the file descriptor to the DVD.
 * See also 'man 3 close'
 *
 * @param dvd_fd DVD device file descriptor (dvd_fd)
 * @return return value, 0 if success, 1 if fail
 */
int dvd_device_close(const int dvd_fd) {

	return close(dvd_fd);

}

/**
 * Check if device is hardware (/dev/dvd, /dev/dvd1, etc.)
 *
 * @param device_filename device filename (/dev/dvd, dvd.iso, etc.)
 * @return bool
 */
bool dvd_device_is_hardware(const char *device_filename) {

	if(strncmp(device_filename, "/dev/", 5) == 0)
		return true;
	else
		return false;

}

/**
 * Check if device is an image (filename)
 *
 * @param device_filename device filename (/dev/dvd, dvd.iso, etc.)
 * @return bool
 */
bool dvd_device_is_image(const char *device_filename) {

	if(dvd_device_is_hardware(device_filename))
		return false;
	else
		return true;

}
