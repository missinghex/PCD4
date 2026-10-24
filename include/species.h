#ifndef SPECIES_H
#define SPECIES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
 * Resolve a Generation IV National Dex species name to its ID.
 *
 * Matching is case-insensitive and ignores spaces, hyphens, periods, and apostrophes.
 * "none" and "blank" resolve to 0. Decimal numeric IDs 0..493 are accepted
 * for compatibility with scripts that used the original CLI.
 */
int species_parse(const char *text, uint16_t *species_id);

/* Returns "None" for ID 0, a species name for 1..493, or NULL otherwise. */
const char *species_name(uint16_t species_id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SPECIES_H
