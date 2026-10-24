#ifndef PK4_H
#define PK4_H

#include <stddef.h>
#include <stdint.h>

#define PK4_STORED_SIZE 136u
#define PK4_PARTY_SIZE 236u

typedef struct {
    uint8_t data[PK4_PARTY_SIZE];
    size_t source_size;
    int source_was_encrypted;
} Pcd4Pk4;

uint16_t pk4_checksum(const uint8_t *decrypted);
int pk4_validate_decrypted(const uint8_t *data, size_t size, char *err, size_t err_size);
int pk4_load(const char *path, Pcd4Pk4 *pk4, char *err, size_t err_size);
void pk4_encrypt(const uint8_t decrypted[PK4_PARTY_SIZE], uint8_t encrypted[PK4_PARTY_SIZE]);
void pk4_decrypt(const uint8_t encrypted[PK4_PARTY_SIZE], uint8_t decrypted[PK4_PARTY_SIZE]);

#endif // PK4_H
