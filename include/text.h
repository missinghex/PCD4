#ifndef TEXT_H
#define TEXT_H

#include <stddef.h>
#include <stdint.h>

/* Generation IV proprietary 16-bit text encoding, Western subset. */
int text_encode(const char *src, uint8_t *dst, size_t dst_bytes, char *err, size_t err_size);
int text_decode(const uint8_t *src, size_t src_bytes, char *dst, size_t dst_size);

#endif // TEXT_H
