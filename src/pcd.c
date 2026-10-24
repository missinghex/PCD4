#include "pcd.h"
#include "common.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int text_field_has_terminator(const uint8_t *data, size_t size) {
    size_t offset;

    for (offset = 0u; offset + 1u < size; offset += 2u) {
        if (read_le16(data + offset) == 0xFFFFu) {
            return 1;
        }
    }

    return 0;
}

void card_init(Pcd4Card *card) {
    memset(card->data, 0, sizeof(card->data));

    /* Gen IV card strings are 16-bit encoded and terminated by 0xFFFF.
     * Fill the whole editable string regions so unused code units are in the
     * conventional empty-string state instead of looking like embedded NULs. */
    memset(card->data + TITLE_OFFSET, 0xFF, TITLE_BYTES);
    memset(card->data + DESCRIPTION_OFFSET, 0xFF, DESCRIPTION_BYTES);
}

int card_load(const char *path, Pcd4Card *card, char *err, size_t err_size) {
    uint8_t *raw = NULL;
    size_t size = 0u;
    Pcd4GiftType type;

    if (!read_file(path, &raw, &size, err, err_size)) {
        return 0;
    }
    if (size != PCD_SIZE) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "PCD must be exactly 856 bytes, got %zu", size);
        }

        free(raw);
        return 0;
    }

    memcpy(card->data, raw, PCD_SIZE);
    free(raw);

    /* PCD stores Pokémon and regular egg gift PK4 data encrypted. Keep the
     * in-memory representation decrypted so validation and edits use one form.
     * A decoded WC4-style payload is accepted when its checksum already works. */
    type = card_type(card);
    if (type == GIFT_POKEMON || type == GIFT_POKEMON_EGG) {
        uint8_t decoded[PK4_PARTY_SIZE];
        uint8_t *gift = card->data + 8u;

        if (!pk4_validate_decrypted(gift, PK4_PARTY_SIZE, NULL, 0u)) {
            pk4_decrypt(gift, decoded);

            if (!pk4_validate_decrypted(decoded, PK4_PARTY_SIZE, err, err_size)) {
                return 0;
            }

            memcpy(gift, decoded, sizeof(decoded));
        }
    }

    return 1;
}

int card_save(const char *path, const Pcd4Card *card, int overwrite, char *err, size_t err_size) {
    Pcd4Card out = *card;
    Pcd4GiftType type = card_type(card);

    if (!card_validate(card, err, err_size)) {
        return 0;
    }

    if (type == GIFT_POKEMON || type == GIFT_POKEMON_EGG) {
        uint8_t encrypted[PK4_PARTY_SIZE];

        pk4_encrypt(card->data + 8u, encrypted);
        memcpy(out.data + 8u, encrypted, sizeof(encrypted));
    }

    return write_file(path, out.data, sizeof(out.data), overwrite, err, err_size);
}

Pcd4GiftType card_type(const Pcd4Card *card) { return (Pcd4GiftType)read_le16(card->data); }

const char *gift_type_name(Pcd4GiftType type) {
    switch (type) {
    case GIFT_NONE:
        return "None";
    case GIFT_POKEMON:
        return "Pokemon";
    case GIFT_POKEMON_EGG:
        return "Pokemon Egg";
    case GIFT_ITEM:
        return "Item";
    case GIFT_RULE:
        return "Rule";
    case GIFT_GOODS:
        return "Goods";
    case GIFT_SUBTYPE:
        return "Seal/Accessory/Backdrop";
    case GIFT_MANAPHY_EGG:
        return "Manaphy Egg";
    case GIFT_MEMBER_CARD:
        return "Member Card";
    case GIFT_OAKS_LETTER:
        return "Oak's Letter";
    case GIFT_AZURE_FLUTE:
        return "Azure Flute";
    case GIFT_POKETCH_APP:
        return "Poketch App";
    case GIFT_SECRET_KEY:
        return "Secret Key";
    case GIFT_POKEMON_MOVIE:
        return "Pokemon Movie";
    case GIFT_POKEWALKER_COURSE:
        return "Pokewalker Course";
    case GIFT_MEMORIAL_PHOTO:
        return "Memorial Photo";
    default:
        return "Unknown";
    }
}

int card_set_type(Pcd4Card *card, Pcd4GiftType type, char *err, size_t err_size) {
    switch (type) {
    case GIFT_POKEMON:
    case GIFT_POKEMON_EGG:
    case GIFT_ITEM:
    case GIFT_SUBTYPE:
    case GIFT_MANAPHY_EGG:
    case GIFT_MEMBER_CARD:
    case GIFT_OAKS_LETTER:
    case GIFT_AZURE_FLUTE:
    case GIFT_POKETCH_APP:
    case GIFT_SECRET_KEY:
    case GIFT_POKEWALKER_COURSE:
        memset(card->data, 0, PGT_SIZE);
        write_le16(card->data, (uint16_t)type);
        return 1;
    default:
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "gift type %u is recognized but not implemented yet.", (unsigned)type);
        }

        return 0;
    }
}

int card_set_item(Pcd4Card *card, uint32_t item_id, char *err, size_t err_size) {
    if (item_id == 0u || item_id > 536u) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "item ID must be in the Generation IV item range 1..536");
        }
        return 0;
    }

    if (!card_set_type(card, GIFT_ITEM, err, err_size)) {
        return 0;
    }

    write_le32(card->data + 4u, item_id);
    return 1;
}

int card_set_subtype(Pcd4Card *card, uint32_t subtype, uint32_t value, char *err, size_t err_size) {
    if (subtype < 1u || subtype > 3u) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "subtype must be 1 (Seal), 2 (Accessory), or 3 (Backdrop)");
        }
        return 0;
    }

    if (subtype == 1u && value == UINT32_MAX) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "seal ID is too large to encode with its +1 storage rule");
        }
        return 0;
    }

    if (!card_set_type(card, GIFT_SUBTYPE, err, err_size)) {
        return 0;
    }

    write_le32(card->data + 4u, subtype);
    write_le32(card->data + 8u, subtype == 1u ? value + 1u : value);
    return 1;
}

int card_set_poketch_app(Pcd4Card *card, uint32_t app_id, char *err, size_t err_size) {
    /* The PGT field is a signed 32-bit ItemID slot. PCD4 intentionally limits
     * authoring to a small nonnegative ID range because it does not yet carry a
     * full Pokétch application catalog for semantic validation. */
    if (app_id > 255u) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "Poketch app ID must be in PCD4's supported range 0..255");
        }
        return 0;
    }

    if (!card_set_type(card, GIFT_POKETCH_APP, err, err_size)) {
        return 0;
    }

    write_le32(card->data + 4u, app_id);
    return 1;
}

int card_set_pokewalker_course(Pcd4Card *card, uint32_t course_id, char *err, size_t err_size) {
    if (course_id > 255u) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "Pokewalker course ID must fit in one byte");
        }
        return 0;
    }

    if (!card_set_type(card, GIFT_POKEWALKER_COURSE, err, err_size)) {
        return 0;
    }

    card->data[4u] = (uint8_t)course_id;
    return 1;
}

int card_set_pk4(Pcd4Card *card, Pcd4GiftType type, const Pcd4Pk4 *pk4, char *err, size_t err_size) {
    if (type != GIFT_POKEMON && type != GIFT_POKEMON_EGG) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "PK4 data can only be used with Pokemon or Pokemon Egg gift types");
        }
        return 0;
    }

    if (!pk4_validate_decrypted(pk4->data, pk4->source_size, err, err_size)) {
        return 0;
    }

    if (!card_set_type(card, type, err, err_size)) {
        return 0;
    }

    memset(card->data + 8u, 0, PK4_PARTY_SIZE);
    memcpy(card->data + 8u, pk4->data, pk4->source_size);
    return 1;
}

int card_set_title(Pcd4Card *card, const char *title, char *err, size_t err_size) { return text_encode(title, card->data + TITLE_OFFSET, TITLE_BYTES, err, err_size); }

int card_get_title(const Pcd4Card *card, char *title, size_t title_size) { return text_decode(card->data + TITLE_OFFSET, TITLE_BYTES, title, title_size); }

int card_set_description(Pcd4Card *card, const char *description, char *err, size_t err_size) { return text_encode(description, card->data + DESCRIPTION_OFFSET, DESCRIPTION_BYTES, err, err_size); }

int card_get_description(const Pcd4Card *card, char *description, size_t description_size) { return text_decode(card->data + DESCRIPTION_OFFSET, DESCRIPTION_BYTES, description, description_size); }

void card_set_card_id(Pcd4Card *card, uint16_t card_id) { write_le16(card->data + CARD_ID_OFFSET, card_id); }

uint16_t card_card_id(const Pcd4Card *card) { return read_le16(card->data + CARD_ID_OFFSET); }

void card_set_compatibility(Pcd4Card *card, uint16_t mask) {
    /* The game mask is the notable endian exception in the PCD card header:
     * its numeric bit values are conventional, but the two bytes are stored
     * big-endian at 0x14C. */
    write_be16(card->data + COMPAT_OFFSET, mask);
}

uint16_t card_compatibility(const Pcd4Card *card) { return read_be16(card->data + COMPAT_OFFSET); }

void card_set_redistribution(Pcd4Card *card, uint8_t limit) { card->data[REDISTRIBUTION_OFFSET] = limit; }

uint8_t card_redistribution(const Pcd4Card *card) { return card->data[REDISTRIBUTION_OFFSET]; }

int card_set_icons(Pcd4Card *card, uint16_t left, uint16_t middle, uint16_t right, char *err, size_t err_size) {
    const uint16_t icons[ICON_COUNT] = {left, middle, right};
    size_t i;

    for (i = 0u; i < ICON_COUNT; ++i) {
        if (icons[i] > MAX_SPECIES_ID) {
            if (err != NULL && err_size > 0u) {
                (void)snprintf(err, err_size, "icon species ID must be 0..%u, got %u", (unsigned)MAX_SPECIES_ID, (unsigned)icons[i]);
            }

            return 0;
        }
    }

    for (i = 0u; i < ICON_COUNT; ++i) {
        write_le16(card->data + ICONS_OFFSET + i * 2u, icons[i]);
    }

    return 1;
}

void card_get_icons(const Pcd4Card *card, uint16_t icons[ICON_COUNT]) {
    size_t i;

    for (i = 0u; i < ICON_COUNT; ++i) {
        icons[i] = read_le16(card->data + ICONS_OFFSET + i * 2u);
    }
}

void card_set_received_days(Pcd4Card *card, uint16_t days) { write_le16(card->data + RECEIVED_OFFSET, days); }

uint16_t card_received_days(const Pcd4Card *card) { return read_le16(card->data + RECEIVED_OFFSET); }

int card_validate(const Pcd4Card *card, char *err, size_t err_size) {
    Pcd4GiftType type = card_type(card);
    uint16_t mask = card_compatibility(card);
    uint16_t icons[ICON_COUNT];
    size_t i;

    if ((mask & (uint16_t)~GAME_MASK) != 0u) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "game compatibility contains unsupported bits: 0x%04X", (unsigned)mask);
        }

        return 0;
    }

    if (mask == 0u) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "at least one compatible game must be selected");
        }

        return 0;
    }

    if (!text_field_has_terminator(card->data + TITLE_OFFSET, TITLE_BYTES)) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "card title has no 0xFFFF terminator");
        }

        return 0;
    }

    if (!text_field_has_terminator(card->data + DESCRIPTION_OFFSET, DESCRIPTION_BYTES)) {
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "card description has no 0xFFFF terminator");
        }

        return 0;
    }

    card_get_icons(card, icons);
    for (i = 0u; i < ICON_COUNT; ++i) {
        if (icons[i] > MAX_SPECIES_ID) {
            if (err != NULL && err_size > 0u) {
                (void)snprintf(err, err_size, "card icon %zu has invalid species ID %u", i + 1u, (unsigned)icons[i]);
            }

            return 0;
        }
    }

    switch (type) {
    case GIFT_POKEMON:
    case GIFT_POKEMON_EGG:
        return pk4_validate_decrypted(card->data + 8u, PK4_PARTY_SIZE, err, err_size);

    case GIFT_ITEM: {
        uint32_t id = read_le32(card->data + 4u);
        if (id == 0u || id > 536u) {
            if (err != NULL && err_size > 0u) {
                (void)snprintf(err, err_size, "invalid item ID %u", (unsigned)id);
            }

            return 0;
        }
        return 1;
    }

    case GIFT_SUBTYPE: {
        uint32_t subtype = read_le32(card->data + 4u);
        if (subtype < 1u || subtype > 3u) {
            if (err != NULL && err_size > 0u) {
                (void)snprintf(err, err_size, "invalid subtype %u", (unsigned)subtype);
            }
            return 0;
        }
        return 1;
    }

    case GIFT_POKETCH_APP:
        if (read_le32(card->data + 4u) > 255u) {
            if (err != NULL && err_size > 0u) {
                (void)snprintf(err, err_size, "Poketch app ID is outside PCD4's supported range");
            }
            return 0;
        }
        return 1;

    case GIFT_MANAPHY_EGG:
    case GIFT_MEMBER_CARD:
    case GIFT_OAKS_LETTER:
    case GIFT_AZURE_FLUTE:
    case GIFT_SECRET_KEY:
    case GIFT_POKEWALKER_COURSE:
        return 1;

    default:
        if (err != NULL && err_size > 0u) {
            (void)snprintf(err, err_size, "gift type %u is not supported for creation/export", (unsigned)type);
        }

        return 0;
    }
}
