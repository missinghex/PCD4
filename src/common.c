#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint16_t read_le16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }

uint16_t read_be16(const uint8_t *p) { return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]); }

uint32_t read_le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

void write_le16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)(v >> 8);
}

void write_be16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xffu);
}

void write_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)(v >> 24);
}

static void set_error(char *err, size_t n, const char *fmt, const char *arg) {
    if (err != NULL && n > 0) {
        (void)snprintf(err, n, fmt, arg);
    }
}

int read_file(const char *path, uint8_t **data, size_t *size, char *err, size_t err_size) {
    FILE *f;
    long length;
    uint8_t *buf;
    size_t got;

    if (path == NULL || data == NULL || size == NULL) {
        set_error(err, err_size, "%s", "invalid read arguments");
        return 0;
    }

    f = fopen(path, "rb");
    if (f == NULL) {
        set_error(err, err_size, "cannot open input: %s", strerror(errno));
        return 0;
    }

    if (fseek(f, 0, SEEK_END) != 0 || (length = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        set_error(err, err_size, "cannot determine input size: %s", strerror(errno));
        fclose(f);
        return 0;
    }

    if ((unsigned long)length > (unsigned long)SIZE_MAX) {
        set_error(err, err_size, "%s", "input file is too large");
        fclose(f);
        return 0;
    }

    buf = (uint8_t *)malloc((size_t)length == 0 ? 1u : (size_t)length);
    if (buf == NULL) {
        set_error(err, err_size, "%s", "out of memory while reading file");
        fclose(f);
        return 0;
    }

    got = fread(buf, 1u, (size_t)length, f);
    if (got != (size_t)length) {
        set_error(err, err_size, "short read: %s", ferror(f) ? strerror(errno) : "unexpected end of file");
        free(buf);
        fclose(f);
        return 0;
    }

    if (fclose(f) != 0) {
        set_error(err, err_size, "cannot close input: %s", strerror(errno));
        free(buf);
        return 0;
    }

    *data = buf;
    *size = (size_t)length;

    return 1;
}

int write_file(const char *path, const uint8_t *data, size_t size, int overwrite, char *err, size_t err_size) {
    FILE *probe;
    FILE *f;
    size_t wrote;

    if (!overwrite) {
        probe = fopen(path, "rb");
        if (probe != NULL) {
            fclose(probe);
            set_error(err, err_size, "%s", "output file already exists (use --force to overwrite)");
            return 0;
        }
    }

    f = fopen(path, "wb");
    if (f == NULL) {
        set_error(err, err_size, "cannot open output: %s", strerror(errno));
        return 0;
    }

    wrote = fwrite(data, 1u, size, f);
    if (wrote != size || fflush(f) != 0) {
        set_error(err, err_size, "write failed: %s", strerror(errno));
        fclose(f);
        return 0;
    }

    if (fclose(f) != 0) {
        set_error(err, err_size, "cannot close output: %s", strerror(errno));
        return 0;
    }

    return 1;
}
