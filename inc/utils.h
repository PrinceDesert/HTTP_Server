#ifndef UTILS_H_
#define UTILS_H_

int get_gmt_time(char *buf, time_t *t, size_t size);
const char *get_filename_ext(const char *filename);
void trim(char *str);

#endif
