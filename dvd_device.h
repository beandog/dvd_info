int dvd_device_access(char *device_filename);
int dvd_device_open(char *device_filename);
int dvd_device_close(int dvd_fd);
bool dvd_device_is_hardware(char *device_filename);
bool dvd_device_is_image(char *device_filename);
