#include "common.h"
#include "pcd.h"
#include "pk4.h"
#include "species.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(expr)                                                                                                                                                                                    \
    do {                                                                                                                                                                                               \
        if (!(expr)) {                                                                                                                                                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr);                                                                                                                            \
            failures++;                                                                                                                                                                                \
        }                                                                                                                                                                                              \
    } while (0)

static void make_pk4(uint8_t data[PK4_PARTY_SIZE], uint32_t pid) {
    size_t i;

    memset(data, 0, PK4_PARTY_SIZE);
    write_le32(data, pid);
    write_le16(data + 4u, 0u);

    for (i = 8u; i < PK4_STORED_SIZE; ++i) {
        data[i] = (uint8_t)((i * 3u + 1u) & 0xFFu);
    }

    write_le16(data + 6u, pk4_checksum(data));

    for (i = PK4_STORED_SIZE; i < PK4_PARTY_SIZE; ++i) {
        data[i] = (uint8_t)(i ^ 0x5Au);
    }
}

static void test_endian(void) {
    uint8_t data[4];

    write_le32(data, 0x78563412u);
    CHECK(data[0] == 0x12u);
    CHECK(data[1] == 0x34u);
    CHECK(data[2] == 0x56u);
    CHECK(data[3] == 0x78u);
    CHECK(read_le32(data) == 0x78563412u);

    write_le16(data, 0xBEEFu);
    CHECK(data[0] == 0xEFu);
    CHECK(data[1] == 0xBEu);
    CHECK(read_le16(data) == 0xBEEFu);

    write_be16(data, 0xBEEFu);
    CHECK(data[0] == 0xBEu);
    CHECK(data[1] == 0xEFu);
    CHECK(read_be16(data) == 0xBEEFu);
}

static void test_text(void) {
    uint8_t encoded[TITLE_BYTES];
    char decoded[64];
    char err[128];
    char too_long[40];

    CHECK(text_encode("Member Card!", encoded, sizeof(encoded), err, sizeof(err)));
    CHECK(text_decode(encoded, sizeof(encoded), decoded, sizeof(decoded)));
    CHECK(strcmp(decoded, "Member Card!") == 0);
    CHECK(read_le16(encoded + 12u) == 0x01DEu);

    memset(too_long, 'A', 36u);
    too_long[36] = '\0';
    CHECK(!text_encode(too_long, encoded, sizeof(encoded), err, sizeof(err)));

    CHECK(text_encode("Line one\\nLine two", encoded, sizeof(encoded), err, sizeof(err)));
    CHECK(read_le16(encoded + 16u) == 0xE000u);
    CHECK(text_decode(encoded, sizeof(encoded), decoded, sizeof(decoded)));
    CHECK(strcmp(decoded, "Line one\nLine two") == 0);

    CHECK(text_encode("caf\xC3\xA9", encoded, sizeof(encoded), err, sizeof(err)));
    CHECK(read_le16(encoded + 6u) == 0x0188u);
    CHECK(text_decode(encoded, sizeof(encoded), decoded, sizeof(decoded)));
    CHECK(strcmp(decoded, "caf\xC3\xA9") == 0);

    /* Valid UTF-8, but U+20AC EURO SIGN is not in the Gen IV map. */
    CHECK(!text_encode("\xE2\x82\xAC", encoded, sizeof(encoded), err, sizeof(err)));
}

static void test_species_names(void) {
    uint16_t species_id = 0u;

    CHECK(species_parse("Darkrai", &species_id));
    CHECK(species_id == 491u);
    CHECK(species_parse("mr mime", &species_id));
    CHECK(species_id == 122u);
    CHECK(species_parse("Mr. Mime", &species_id));
    CHECK(species_id == 122u);
    CHECK(species_parse("ho-oh", &species_id));
    CHECK(species_id == 250u);
    CHECK(species_parse("Porygon Z", &species_id));
    CHECK(species_id == 474u);
    CHECK(species_parse("none", &species_id));
    CHECK(species_id == 0u);
    CHECK(species_parse("483", &species_id));
    CHECK(species_id == 483u);
    CHECK(strcmp(species_name(491u), "Darkrai") == 0);
    CHECK(strcmp(species_name(0u), "None") == 0);
    CHECK(species_name(494u) == NULL);
    CHECK(!species_parse("MissingNo", &species_id));
}

static void test_pk4_crypto(void) {
    uint8_t plain[PK4_PARTY_SIZE];
    uint8_t encrypted[PK4_PARTY_SIZE];
    uint8_t decrypted[PK4_PARTY_SIZE];
    unsigned sv;

    for (sv = 0u; sv < 32u; ++sv) {
        uint32_t pid = (uint32_t)((sv << 13) | 1u);
        make_pk4(plain, pid);
        pk4_encrypt(plain, encrypted);
        CHECK(memcmp(plain, encrypted, sizeof(plain)) != 0);
        pk4_decrypt(encrypted, decrypted);
        CHECK(memcmp(plain, decrypted, sizeof(plain)) == 0);
        CHECK(pk4_validate_decrypted(decrypted, sizeof(decrypted), NULL, 0u));
    }
}

static void test_pk4_fixed_vector(void) {
    static const uint8_t expected_main[16] = {
        0x4F, 0x3B, 0x56, 0x88, 0x05, 0xE9, 0xB1, 0xA4, 0xED, 0x60, 0xA1, 0x3A, 0x79, 0x34, 0xB8, 0x54,
    };

    static const uint8_t expected_party[16] = {
        0xFD, 0x4C, 0xD8, 0x77, 0x64, 0x2E, 0x89, 0x5A, 0x30, 0x3F, 0x6E, 0x9E, 0xD2, 0x85, 0xA9, 0xA6,
    };

    uint8_t plain[PK4_PARTY_SIZE];
    uint8_t encrypted[PK4_PARTY_SIZE];

    /* PID 0x6001 selects shuffle value 3, which exercises a nontrivial order. */
    make_pk4(plain, 0x00006001u);
    CHECK(read_le16(plain + 6u) == 0x5C80u);
    pk4_encrypt(plain, encrypted);

    CHECK(memcmp(encrypted + 8u, expected_main, sizeof(expected_main)) == 0);
    CHECK(memcmp(encrypted + PK4_STORED_SIZE, expected_party, sizeof(expected_party)) == 0);
}

static void test_pk4_file_loading(void) {
    const char *plain_party_path = "test_plain_236.pk4";
    const char *encrypted_party_path = "test_encrypted_236.pk4";
    const char *plain_stored_path = "test_plain_136.pk4";
    const char *encrypted_stored_path = "test_encrypted_136.pk4";
    const char *bad_path = "test_bad.pk4";
    uint8_t plain[PK4_PARTY_SIZE];
    uint8_t encrypted[PK4_PARTY_SIZE];
    uint8_t bad[100] = {0};
    Pcd4Pk4 loaded;
    char err[160];

    remove(plain_party_path);
    remove(encrypted_party_path);
    remove(plain_stored_path);
    remove(encrypted_stored_path);
    remove(bad_path);

    make_pk4(plain, 0x00012001u);
    pk4_encrypt(plain, encrypted);

    CHECK(write_file(plain_party_path, plain, sizeof(plain), 1, err, sizeof(err)));
    CHECK(pk4_load(plain_party_path, &loaded, err, sizeof(err)));
    CHECK(loaded.source_size == PK4_PARTY_SIZE);
    CHECK(loaded.source_was_encrypted == 0);
    CHECK(memcmp(loaded.data, plain, sizeof(plain)) == 0);

    CHECK(write_file(encrypted_party_path, encrypted, sizeof(encrypted), 1, err, sizeof(err)));
    CHECK(pk4_load(encrypted_party_path, &loaded, err, sizeof(err)));
    CHECK(loaded.source_size == PK4_PARTY_SIZE);
    CHECK(loaded.source_was_encrypted == 1);
    CHECK(memcmp(loaded.data, plain, sizeof(plain)) == 0);

    CHECK(write_file(plain_stored_path, plain, PK4_STORED_SIZE, 1, err, sizeof(err)));
    CHECK(pk4_load(plain_stored_path, &loaded, err, sizeof(err)));
    CHECK(loaded.source_size == PK4_STORED_SIZE);
    CHECK(loaded.source_was_encrypted == 0);
    CHECK(memcmp(loaded.data, plain, PK4_STORED_SIZE) == 0);

    CHECK(write_file(encrypted_stored_path, encrypted, PK4_STORED_SIZE, 1, err, sizeof(err)));
    CHECK(pk4_load(encrypted_stored_path, &loaded, err, sizeof(err)));
    CHECK(loaded.source_size == PK4_STORED_SIZE);
    CHECK(loaded.source_was_encrypted == 1);
    CHECK(memcmp(loaded.data, plain, PK4_STORED_SIZE) == 0);

    CHECK(write_file(bad_path, bad, sizeof(bad), 1, err, sizeof(err)));
    CHECK(!pk4_load(bad_path, &loaded, err, sizeof(err)));

    remove(plain_party_path);
    remove(encrypted_party_path);
    remove(plain_stored_path);
    remove(encrypted_stored_path);
    remove(bad_path);
}

static void test_card_metadata(void) {
    Pcd4Card card;
    char err[128];
    char title[64];
    char description[512];
    uint16_t icons[ICON_COUNT];

    card_init(&card);
    CHECK(read_le16(card.data + TITLE_OFFSET) == 0xFFFFu);
    CHECK(read_le16(card.data + DESCRIPTION_OFFSET) == 0xFFFFu);
    CHECK(card_set_type(&card, GIFT_AZURE_FLUTE, err, sizeof(err)));
    CHECK(read_le16(card.data) == GIFT_AZURE_FLUTE);
    CHECK(read_le32(card.data + 4u) == 0u);
    CHECK(card_set_title(&card, "Azure Flute", err, sizeof(err)));
    CHECK(card_set_description(&card, "A verified event description.", err, sizeof(err)));
    card_set_card_id(&card, 42u);
    card_set_compatibility(&card, GAME_DIAMOND | GAME_PLATINUM);
    card_set_redistribution(&card, 255u);
    CHECK(card_set_icons(&card, 491u, 0u, 483u, err, sizeof(err)));
    card_set_received_days(&card, 8982u);

    CHECK(card_validate(&card, err, sizeof(err)));
    CHECK(card_get_title(&card, title, sizeof(title)));
    CHECK(strcmp(title, "Azure Flute") == 0);
    CHECK(card_get_description(&card, description, sizeof(description)));
    CHECK(strcmp(description, "A verified event description.") == 0);
    CHECK(card_card_id(&card) == 42u);
    CHECK(card_compatibility(&card) == (GAME_DIAMOND | GAME_PLATINUM));
    CHECK(card.data[COMPAT_OFFSET] == 0x00u);
    CHECK(card.data[COMPAT_OFFSET + 1u] == 0x14u);
    CHECK(read_be16(card.data + COMPAT_OFFSET) == 0x0014u);
    CHECK(card_redistribution(&card) == 255u);
    card_get_icons(&card, icons);
    CHECK(icons[0] == 491u);
    CHECK(icons[1] == 0u);
    CHECK(icons[2] == 483u);
    CHECK(read_le16(card.data + ICONS_OFFSET) == 491u);
    CHECK(card_received_days(&card) == 8982u);
    CHECK(read_le16(card.data + RECEIVED_OFFSET) == 8982u);

    CHECK(!card_set_icons(&card, 494u, 0u, 0u, err, sizeof(err)));

    card_set_compatibility(&card, 0u);
    CHECK(!card_validate(&card, err, sizeof(err)));
    card_set_compatibility(&card, 0x0002u);
    CHECK(!card_validate(&card, err, sizeof(err)));

    card_set_compatibility(&card, GAME_PLATINUM);
    memset(card.data + DESCRIPTION_OFFSET, 0, DESCRIPTION_BYTES);
    CHECK(!card_validate(&card, err, sizeof(err)));
}

static void test_gift_specific_fields(void) {
    Pcd4Card card;
    char err[128];

    card_init(&card);
    CHECK(card_set_item(&card, 467u, err, sizeof(err)));
    CHECK(card_type(&card) == GIFT_ITEM);
    CHECK(read_le32(card.data + 4u) == 467u);
    CHECK(!card_set_item(&card, 0u, err, sizeof(err)));
    CHECK(!card_set_item(&card, 537u, err, sizeof(err)));

    CHECK(card_set_subtype(&card, 1u, 0u, err, sizeof(err)));
    CHECK(read_le32(card.data + 4u) == 1u);
    CHECK(read_le32(card.data + 8u) == 1u);
    CHECK(card_set_subtype(&card, 2u, 9u, err, sizeof(err)));
    CHECK(read_le32(card.data + 8u) == 9u);
    CHECK(!card_set_subtype(&card, 4u, 0u, err, sizeof(err)));

    CHECK(card_set_poketch_app(&card, 25u, err, sizeof(err)));
    CHECK(read_le32(card.data + 4u) == 25u);
    CHECK(!card_set_poketch_app(&card, 256u, err, sizeof(err)));

    CHECK(card_set_pokewalker_course(&card, 7u, err, sizeof(err)));
    CHECK(card.data[4u] == 7u);
    CHECK(!card_set_pokewalker_course(&card, 256u, err, sizeof(err)));

    CHECK(!card_set_type(&card, GIFT_RULE, err, sizeof(err)));
    CHECK(!card_set_type(&card, GIFT_GOODS, err, sizeof(err)));
    CHECK(!card_set_type(&card, GIFT_POKEMON_MOVIE, err, sizeof(err)));
    CHECK(!card_set_type(&card, GIFT_MEMORIAL_PHOTO, err, sizeof(err)));
}

static void test_pokemon_pcd_round_trip(void) {
    const char *path = "test_pokemon.pcd";
    uint8_t plain[PK4_PARTY_SIZE];
    uint8_t *raw = NULL;
    size_t raw_size = 0u;
    Pcd4Pk4 pk4;
    Pcd4Card card;
    Pcd4Card loaded;
    char err[160];
    char title[64];

    remove(path);
    make_pk4(plain, 0x00024001u);
    memset(&pk4, 0, sizeof(pk4));
    memcpy(pk4.data, plain, sizeof(plain));
    pk4.source_size = sizeof(plain);

    card_init(&card);
    CHECK(card_set_pk4(&card, GIFT_POKEMON, &pk4, err, sizeof(err)));
    CHECK(card_set_title(&card, "Test Pokemon", err, sizeof(err)));
    card_set_card_id(&card, 77u);
    card_set_compatibility(&card, GAME_DIAMOND | GAME_PEARL | GAME_PLATINUM);

    CHECK(card_save(path, &card, 0, err, sizeof(err)));
    CHECK(!card_save(path, &card, 0, err, sizeof(err)));
    CHECK(read_file(path, &raw, &raw_size, err, sizeof(err)));
    CHECK(raw_size == PCD_SIZE);
    if (raw != NULL && raw_size == PCD_SIZE) {
        CHECK(memcmp(raw + 8u, plain, sizeof(plain)) != 0);
        free(raw);
        raw = NULL;
    }

    CHECK(card_load(path, &loaded, err, sizeof(err)));
    CHECK(card_validate(&loaded, err, sizeof(err)));
    CHECK(memcmp(loaded.data + 8u, plain, sizeof(plain)) == 0);
    CHECK(card_card_id(&loaded) == 77u);
    CHECK(card_get_title(&loaded, title, sizeof(title)));
    CHECK(strcmp(title, "Test Pokemon") == 0);

    remove(path);
}

static void test_bad_pcd_size(void) {
    const char *path = "test_bad.pcd";
    uint8_t bytes[64] = {0};
    Pcd4Card card;
    char err[128];

    remove(path);
    CHECK(write_file(path, bytes, sizeof(bytes), 1, err, sizeof(err)));
    CHECK(!card_load(path, &card, err, sizeof(err)));
    remove(path);
}

int main(void) {
    test_endian();
    test_text();
    test_species_names();
    test_pk4_crypto();
    test_pk4_fixed_vector();
    test_pk4_file_loading();
    test_card_metadata();
    test_gift_specific_fields();
    test_pokemon_pcd_round_trip();
    test_bad_pcd_size();

    if (failures != 0) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    puts("all tests passed");
    return 0;
}
