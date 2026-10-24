#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>

uint16_t read_le16(const uint8_t *p);
uint16_t read_be16(const uint8_t *p);
uint32_t read_le32(const uint8_t *p);
void write_le16(uint8_t *p, uint16_t v);
void write_be16(uint8_t *p, uint16_t v);
void write_le32(uint8_t *p, uint32_t v);
int read_file(const char *path, uint8_t **data, size_t *size, char *err, size_t err_size);
int write_file(const char *path, const uint8_t *data, size_t size, int overwrite, char *err, size_t err_size);

#endif // COMMON_H
