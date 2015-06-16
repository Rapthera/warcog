#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

char* file_text(const char *path);
void* file_raw(const char *path);
uint32_t inflate(void *dest, void *src, uint32_t dest_size, uint32_t src_len);

#endif
