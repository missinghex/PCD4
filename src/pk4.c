#include "pk4.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Generation IV derives a 5-bit shuffle value from the personality value.
 * Values 24-31 deliberately repeat the first eight layouts. The inverse
 * table is used before encryption so the same shuffle routine can serve
 * both encryption and decryption.
 */
static const uint8_t block_position[32][4] = {{0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 3, 1, 2}, {0, 2, 3, 1}, {0, 3, 2, 1}, {1, 0, 2, 3}, {1, 0, 3, 2}, {2, 0, 1, 3}, {3, 0, 1, 2}, {2, 0, 3, 1},
                                              {3, 0, 2, 1}, {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 1, 0, 3}, {3, 1, 0, 2}, {2, 3, 0, 1}, {3, 2, 0, 1}, {1, 2, 3, 0}, {1, 3, 2, 0}, {2, 1, 3, 0}, {3, 1, 2, 0},
                                              {2, 3, 1, 0}, {3, 2, 1, 0}, {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 3, 1, 2}, {0, 2, 3, 1}, {0, 3, 2, 1}, {1, 0, 2, 3}, {1, 0, 3, 2}};

static const uint8_t block_position_invert[32] = {0, 1, 2, 4, 3, 5, 6, 7, 12, 18, 13, 19, 8, 10, 14, 20, 16, 22, 9, 11, 15, 21, 17, 23, 0, 1, 2, 4, 3, 5, 6, 7};

static void swap_blocks(uint8_t *data, unsigned a, unsigned b) {
    uint8_t tmp[32];

    memcpy(tmp, data + (a * 32u), sizeof(tmp));
    memcpy(data + (a * 32u), data + (b * 32u), sizeof(tmp));
    memcpy(data + (b * 32u), tmp, sizeof(tmp));
}

static void shuffle_blocks(uint8_t *data, unsigned sv) {
    uint8_t perm[4] = {0, 1, 2, 3};
    uint8_t slot_of[4] = {0, 1, 2, 3};
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        uint8_t desired = block_position[sv][i];
        uint8_t j = slot_of[desired];
        uint8_t block_at_i;

        if (j == i) {
            continue;
        }

        swap_blocks(data, i, j);
        block_at_i = perm[i];
        perm[i] = desired;
        perm[j] = block_at_i;
        slot_of[desired] = (uint8_t)i;
        slot_of[block_at_i] = j;
    }
}

static void crypt_words(uint8_t *data, size_t size, uint32_t seed) {
    size_t i;

    for (i = 0; i + 1u < size; i += 2u) {
        uint16_t word;
        seed = seed * 0x41C64E6Du + 0x6073u;
        word = read_le16(data + i);
        word ^= (uint16_t)(seed >> 16);
        write_le16(data + i, word);
    }
}

uint16_t pk4_checksum(const uint8_t *decrypted) {
    uint32_t sum = 0u;
    size_t i;

    for (i = 8u; i < PK4_STORED_SIZE; i += 2u)
        sum += read_le16(decrypted + i);

    return (uint16_t)sum;
}

int pk4_validate_decrypted(const uint8_t *data, size_t size, char *err, size_t err_size) {
    uint16_t expected;
    uint16_t actual;

    if (size != PK4_STORED_SIZE && size != PK4_PARTY_SIZE) {
        if (err && err_size)
            (void)snprintf(err, err_size, "PK4 must be 136 or 236 bytes, got %zu", size);
        return 0;
    }

    expected = read_le16(data + 6u);
    actual = pk4_checksum(data);

    if (expected != actual) {
        if (err && err_size)
            (void)snprintf(err, err_size, "PK4 checksum mismatch: stored %04X, calculated %04X", expected, actual);

        return 0;
    }

    return 1;
}

void pk4_decrypt(const uint8_t encrypted[PK4_PARTY_SIZE], uint8_t decrypted[PK4_PARTY_SIZE]) {
    uint32_t pid;
    uint16_t checksum;
    memcpy(decrypted, encrypted, PK4_PARTY_SIZE);
    pid = read_le32(decrypted);
    checksum = read_le16(decrypted + 6u);
    crypt_words(decrypted + 8u, 128u, checksum);
    shuffle_blocks(decrypted + 8u, (unsigned)((pid >> 13) & 31u));
    crypt_words(decrypted + PK4_STORED_SIZE, PK4_PARTY_SIZE - PK4_STORED_SIZE, pid);
}

void pk4_encrypt(const uint8_t decrypted[PK4_PARTY_SIZE], uint8_t encrypted[PK4_PARTY_SIZE]) {
    uint32_t pid;
    uint16_t checksum;
    memcpy(encrypted, decrypted, PK4_PARTY_SIZE);
    pid = read_le32(encrypted);
    checksum = read_le16(encrypted + 6u);
    shuffle_blocks(encrypted + 8u, block_position_invert[(pid >> 13) & 31u]);
    crypt_words(encrypted + 8u, 128u, checksum);
    crypt_words(encrypted + PK4_STORED_SIZE, PK4_PARTY_SIZE - PK4_STORED_SIZE, pid);
}

int pk4_load(const char *path, Pcd4Pk4 *pk4, char *err, size_t err_size) {
    uint8_t *raw = NULL;
    size_t size = 0u;
    uint8_t party[PK4_PARTY_SIZE];
    uint8_t dec[PK4_PARTY_SIZE];
    int raw_valid;
    int dec_valid;

    if (!read_file(path, &raw, &size, err, err_size))
        return 0;

    if (size != PK4_STORED_SIZE && size != PK4_PARTY_SIZE) {
        if (err && err_size)
            (void)snprintf(err, err_size, "PK4 file has invalid size %zu (expected 136 or 236)", size);

        free(raw);
        return 0;
    }

    memset(party, 0, sizeof(party));
    memcpy(party, raw, size);
    free(raw);
    raw_valid = pk4_validate_decrypted(party, size, NULL, 0u);
    pk4_decrypt(party, dec);
    dec_valid = pk4_validate_decrypted(dec, size, NULL, 0u);

    if (!raw_valid && !dec_valid) {
        if (err && err_size)
            (void)snprintf(err, err_size, "PK4 is neither a valid decrypted file nor a valid encrypted file (checksum failed)");

        return 0;
    }

    memset(pk4, 0, sizeof(*pk4));
    pk4->source_size = size;

    if (raw_valid) {
        memcpy(pk4->data, party, size);
        pk4->source_was_encrypted = 0;
    } else {
        memcpy(pk4->data, dec, size);
        pk4->source_was_encrypted = 1;
    }

    return 1;
}
