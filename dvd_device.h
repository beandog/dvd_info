#ifndef _DVD_DEVICE_H_
#define _DVD_DEVICE_H_

/**
 * @file dvd_device.h
 *
 * @brief functions to access a DVD device or file (DVD-ROM, .ISO, .UDF)
 *
 */

/**
 * Check to see if the file can actually be accessed by the user running
 * the program.  This would check to see if the file exists as well.
 *
 * @param device_filename filename (/dev/dvd, dvd.iso, etc.)
 * @return bool can access device or not
 */
bool dvd_device_access(const char *device_filename);

/**
 * Use open() to create a file descriptor to the DVD.
 * See also 'man 3 open'
 *
 * @param device_filename device filename (/dev/dvd, dvd.iso, etc.)
 * @return file descriptor, -1 if failed
 */
int dvd_device_open(const char *device_filename);

/**
 * Use close() to close the file descriptor to the DVD.
 * See also 'man 3 close'
 *
 * @param dvd_fd DVD device file descriptor (dvd_fd)
 * @return return value, 0 if success, 1 if fail
 */
int dvd_device_close(const int dvd_fd);

/**
 * Check if device is hardware (/dev/dvd, /dev/dvd1, etc.)
 *
 * @param device_filename device filename (/dev/dvd, dvd.iso, etc.)
 * @return bool
 */
bool dvd_device_is_hardware(const char *device_filename);

/**
 * Check if device is an image (filename)
 *
 * @param device_filename device filename (/dev/dvd, dvd.iso, etc.)
 * @return bool
 */
bool dvd_device_is_image(const char *device_filename);

// _DVD_DEVICE_H_
#endif
