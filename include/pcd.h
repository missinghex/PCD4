#ifndef PCD_H
#define PCD_H

#include <stddef.h>
#include <stdint.h>

#include "pk4.h"

#define PGT_SIZE 0x104u
#define PCD_SIZE 0x358u

#define TITLE_OFFSET 0x104u
#define TITLE_BYTES 0x48u
#define COMPAT_OFFSET 0x14Cu
#define CARD_ID_OFFSET 0x150u
#define DESCRIPTION_OFFSET 0x154u
#define DESCRIPTION_BYTES 0x1F4u
#define REDISTRIBUTION_OFFSET 0x348u
#define ICONS_OFFSET 0x34Au
#define ICON_COUNT 3u
#define RECEIVED_OFFSET 0x354u
#define MAX_SPECIES_ID 493u

typedef enum {
    GIFT_NONE = 0,
    GIFT_POKEMON = 1,
    GIFT_POKEMON_EGG = 2,
    GIFT_ITEM = 3,
    GIFT_RULE = 4,
    GIFT_GOODS = 5,
    GIFT_SUBTYPE = 6,
    GIFT_MANAPHY_EGG = 7,
    GIFT_MEMBER_CARD = 8,
    GIFT_OAKS_LETTER = 9,
    GIFT_AZURE_FLUTE = 10,
    GIFT_POKETCH_APP = 11,
    GIFT_SECRET_KEY = 12,
    GIFT_POKEMON_MOVIE = 13,
    GIFT_POKEWALKER_COURSE = 14,
    GIFT_MEMORIAL_PHOTO = 15
} Pcd4GiftType;

enum {
    GAME_SOULSILVER = 0x0001u,
    GAME_DIAMOND = 0x0004u,
    GAME_PEARL = 0x0008u,
    GAME_PLATINUM = 0x0010u,
    GAME_HEARTGOLD = 0x8000u,
    GAME_MASK = 0x801Du
};

typedef struct {
    uint8_t data[PCD_SIZE];
} Pcd4Card;

void card_init(Pcd4Card *card);
int card_load(const char *path, Pcd4Card *card, char *err, size_t err_size);
int card_save(const char *path, const Pcd4Card *card, int overwrite,
                   char *err, size_t err_size);

Pcd4GiftType card_type(const Pcd4Card *card);
const char *gift_type_name(Pcd4GiftType type);
int card_set_type(Pcd4Card *card, Pcd4GiftType type,
                       char *err, size_t err_size);
int card_set_item(Pcd4Card *card, uint32_t item_id,
                       char *err, size_t err_size);
int card_set_subtype(Pcd4Card *card, uint32_t subtype, uint32_t value,
                          char *err, size_t err_size);
int card_set_poketch_app(Pcd4Card *card, uint32_t app_id,
                              char *err, size_t err_size);
int card_set_pokewalker_course(Pcd4Card *card, uint32_t course_id,
                                    char *err, size_t err_size);
int card_set_pk4(Pcd4Card *card, Pcd4GiftType type, const Pcd4Pk4 *pk4,
                      char *err, size_t err_size);

int card_set_title(Pcd4Card *card, const char *title,
                        char *err, size_t err_size);
int card_get_title(const Pcd4Card *card, char *title, size_t title_size);
int card_set_description(Pcd4Card *card, const char *description,
                              char *err, size_t err_size);
int card_get_description(const Pcd4Card *card, char *description,
                              size_t description_size);
void card_set_card_id(Pcd4Card *card, uint16_t card_id);
uint16_t card_card_id(const Pcd4Card *card);
void card_set_compatibility(Pcd4Card *card, uint16_t mask);
uint16_t card_compatibility(const Pcd4Card *card);
void card_set_redistribution(Pcd4Card *card, uint8_t limit);
uint8_t card_redistribution(const Pcd4Card *card);
int card_set_icons(Pcd4Card *card, uint16_t left, uint16_t middle,
                        uint16_t right, char *err, size_t err_size);
void card_get_icons(const Pcd4Card *card, uint16_t icons[ICON_COUNT]);
void card_set_received_days(Pcd4Card *card, uint16_t days);
uint16_t card_received_days(const Pcd4Card *card);

int card_validate(const Pcd4Card *card, char *err, size_t err_size);

#endif
