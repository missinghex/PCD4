#include "common.h"
#include "pcd.h"
#include "species.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_SIZE 256u
#define INPUT_SIZE (DESCRIPTION_BYTES * 2u)

typedef struct {
    Pcd4GiftType type;
    const char *pk4_path;
    const char *output_path;
    uint32_t item_id;
    uint32_t subtype;
    uint32_t subtype_value;
    uint32_t app_id;
    uint32_t course_id;
    int have_type;
    int have_item;
    int have_subtype;
    int have_subtype_value;
    int have_app;
    int have_course;
    int force;
} CreateOptions;

static int prompt_line(const char *label, char *buffer, size_t size);

static void usage(void) {
    puts("PCD4 - Pokemon Generation IV Mystery Gift creator\n"
         "\n"
         "Usage:\n"
         "  pcd4 interactive\n"
         "  pcd4 create --type TYPE [gift options] [metadata options] -o FILE [--force]\n"
         "  pcd4 edit INPUT.pcd [metadata options] -o FILE [--force]\n"
         "  pcd4 inspect INPUT.pcd\n"
         "  pcd4 validate INPUT.pcd\n"
         "\n"
         "Types: pokemon, egg, item, subtype, manaphy, member-card, oaks-letter,\n"
         "       azure-flute, poketch-app, secret-key, pokewalker-course\n"
         "Gift options: --pk4 FILE, --item ID, --subtype 1|2|3 --value N,\n"
         "              --app ID, --course ID\n"
         "Metadata: --title TEXT --description TEXT --card-id N --games LIST\n"
         "          --redistribution N --icons NAME,NAME,NAME --received YYYY-MM-DD\n"
         "Games LIST: comma-separated d,p,pt,hg,ss (example: d,p,pt)\n"
         "Icons accept Pokemon names or 'none' (example: darkrai,none,dialga).\n"
         "Numeric species IDs 0..493 are also accepted.\n"
         "Redistribution is 0..255.\n");
}

static int parse_u32(const char *text, uint32_t *value) {
    char *end = NULL;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed > 0xFFFFFFFFul) {
        return 0;
    }

    *value = (uint32_t)parsed;
    return 1;
}

static int str_ieq(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }

        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int parse_type(const char *text, Pcd4GiftType *type) {
    if (str_ieq(text, "pokemon")) {
        *type = GIFT_POKEMON;
    } else if (str_ieq(text, "egg")) {
        *type = GIFT_POKEMON_EGG;
    } else if (str_ieq(text, "item")) {
        *type = GIFT_ITEM;
    } else if (str_ieq(text, "subtype")) {
        *type = GIFT_SUBTYPE;
    } else if (str_ieq(text, "manaphy")) {
        *type = GIFT_MANAPHY_EGG;
    } else if (str_ieq(text, "member-card")) {
        *type = GIFT_MEMBER_CARD;
    } else if (str_ieq(text, "oaks-letter")) {
        *type = GIFT_OAKS_LETTER;
    } else if (str_ieq(text, "azure-flute")) {
        *type = GIFT_AZURE_FLUTE;
    } else if (str_ieq(text, "poketch-app")) {
        *type = GIFT_POKETCH_APP;
    } else if (str_ieq(text, "secret-key")) {
        *type = GIFT_SECRET_KEY;
    } else if (str_ieq(text, "pokewalker-course")) {
        *type = GIFT_POKEWALKER_COURSE;
    } else {
        return 0;
    }

    return 1;
}

static int parse_games(const char *text, uint16_t *mask) {
    char buffer[128];
    char *token;
    uint16_t parsed = 0u;

    if (strlen(text) >= sizeof(buffer)) {
        return 0;
    }

    strcpy(buffer, text);

    token = strtok(buffer, ",");
    while (token != NULL) {
        if (str_ieq(token, "d") || str_ieq(token, "diamond")) {
            parsed |= GAME_DIAMOND;
        } else if (str_ieq(token, "p") || str_ieq(token, "pearl")) {
            parsed |= GAME_PEARL;
        } else if (str_ieq(token, "pt") || str_ieq(token, "platinum")) {
            parsed |= GAME_PLATINUM;
        } else if (str_ieq(token, "hg") || str_ieq(token, "heartgold")) {
            parsed |= GAME_HEARTGOLD;
        } else if (str_ieq(token, "ss") || str_ieq(token, "soulsilver")) {
            parsed |= GAME_SOULSILVER;
        } else {
            return 0;
        }

        token = strtok(NULL, ",");
    }

    if (parsed == 0u) {
        return 0;
    }

    *mask = parsed;
    return 1;
}

static void print_games(uint16_t mask) {
    struct GameName {
        uint16_t bit;
        const char *name;
    };

    static const struct GameName games[] = {
        {GAME_DIAMOND, "Diamond"}, {GAME_PEARL, "Pearl"}, {GAME_PLATINUM, "Platinum"}, {GAME_HEARTGOLD, "HeartGold"}, {GAME_SOULSILVER, "SoulSilver"},
    };

    size_t i;
    int first = 1;

    for (i = 0u; i < sizeof(games) / sizeof(games[0]); ++i) {
        if ((mask & games[i].bit) != 0u) {
            printf("%s%s", first ? "" : ", ", games[i].name);
            first = 0;
        }
    }
}

static int parse_icons(const char *text, uint16_t icons[ICON_COUNT]) {
    char buffer[160];
    char *token;
    size_t count = 0u;

    if (strlen(text) >= sizeof(buffer)) {
        return 0;
    }

    strcpy(buffer, text);

    token = strtok(buffer, ",");
    while (token != NULL) {
        uint16_t species_id;
        if (count >= ICON_COUNT || !species_parse(token, &species_id)) {
            return 0;
        }

        icons[count++] = species_id;
        token = strtok(NULL, ",");
    }
    return count == ICON_COUNT;
}

static int prompt_icon(const char *prompt, uint16_t *species_id) {
    char line[INPUT_SIZE];

    if (!prompt_line(prompt, line, sizeof(line))) {
        return 0;
    }

    if (line[0] == '\0') {
        *species_id = 0u;
        return 1;
    }

    if (!species_parse(line, species_id)) {
        fprintf(stderr, "error: unknown Pokemon '%s'; enter a Gen IV species name or none\n", line);
        return 0;
    }

    return 1;
}

static int is_leap_year(unsigned year) { return year % 4u == 0u && (year % 100u != 0u || year % 400u == 0u); }

static unsigned days_in_month(unsigned year, unsigned month) {
    static const unsigned days[] = {
        31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u,
    };

    if (month == 2u && is_leap_year(year)) {
        return 29u;
    }

    return days[month - 1u];
}

static int parse_received_date(const char *text, uint16_t *days_out) {
    unsigned year;
    unsigned month;
    unsigned day;
    unsigned y;
    unsigned m;
    uint32_t days = 0u;
    char trailing;

    if (sscanf(text, "%u-%u-%u%c", &year, &month, &day, &trailing) != 3 || year < 2000u || month < 1u || month > 12u || day < 1u || day > days_in_month(year, month)) {
        return 0;
    }

    for (y = 2000u; y < year; ++y) {
        days += is_leap_year(y) ? 366u : 365u;
        if (days > 65535u) {
            return 0;
        }
    }

    for (m = 1u; m < month; ++m) {
        days += days_in_month(year, m);
    }

    days += day - 1u;
    if (days > 65535u) {
        return 0;
    }

    *days_out = (uint16_t)days;
    return 1;
}

static void format_received_date(uint16_t days, char out[11]) {
    uint32_t remaining = days;
    unsigned year = 2000u;
    unsigned month = 1u;
    unsigned year_days;
    unsigned month_days;

    for (;;) {
        year_days = is_leap_year(year) ? 366u : 365u;
        if (remaining < year_days) {
            break;
        }
        remaining -= year_days;
        ++year;
    }

    for (;;) {
        month_days = days_in_month(year, month);
        if (remaining < month_days) {
            break;
        }
        remaining -= month_days;
        ++month;
    }

    {
        unsigned day = (unsigned)remaining + 1u;
        out[0] = (char)('0' + (year / 1000u) % 10u);
        out[1] = (char)('0' + (year / 100u) % 10u);
        out[2] = (char)('0' + (year / 10u) % 10u);
        out[3] = (char)('0' + year % 10u);
        out[4] = '-';
        out[5] = (char)('0' + month / 10u);
        out[6] = (char)('0' + month % 10u);
        out[7] = '-';
        out[8] = (char)('0' + day / 10u);
        out[9] = (char)('0' + day % 10u);
        out[10] = '\0';
    }
}

static int require_value(int argc, char **argv, int *index, const char **value) {
    if (*index + 1 >= argc) {
        fprintf(stderr, "error: %s requires a value\n", argv[*index]);
        return 0;
    }

    ++*index;
    *value = argv[*index];
    return 1;
}

static int parse_metadata_option(Pcd4Card *card, int argc, char **argv, int *index, const char **output_path, int *force) {
    const char *value;
    char err[ERROR_SIZE];

    if (strcmp(argv[*index], "--title") == 0) {
        if (!require_value(argc, argv, index, &value)) {
            return -1;
        }
        if (!card_set_title(card, value, err, sizeof(err))) {
            fprintf(stderr, "error: %s\n", err);
            return -1;
        }
        return 1;
    }

    if (strcmp(argv[*index], "--description") == 0) {
        if (!require_value(argc, argv, index, &value)) {
            return -1;
        }
        if (!card_set_description(card, value, err, sizeof(err))) {
            fprintf(stderr, "error: %s\n", err);
            return -1;
        }
        return 1;
    }

    if (strcmp(argv[*index], "--card-id") == 0) {
        uint32_t id;
        if (!require_value(argc, argv, index, &value)) {
            return -1;
        }
        if (!parse_u32(value, &id) || id > 65535u) {
            fputs("error: card ID must be in the range 0..65535\n", stderr);
            return -1;
        }
        card_set_card_id(card, (uint16_t)id);
        return 1;
    }

    if (strcmp(argv[*index], "--games") == 0) {
        uint16_t games;
        if (!require_value(argc, argv, index, &value)) {
            return -1;
        }
        if (!parse_games(value, &games)) {
            fputs("error: invalid --games list\n", stderr);
            return -1;
        }
        card_set_compatibility(card, games);
        return 1;
    }

    if (strcmp(argv[*index], "--redistribution") == 0) {
        uint32_t limit;
        if (!require_value(argc, argv, index, &value)) {
            return -1;
        }
        if (!parse_u32(value, &limit) || limit > 255u) {
            fputs("error: redistribution limit must be in the range 0..255\n", stderr);
            return -1;
        }
        card_set_redistribution(card, (uint8_t)limit);
        return 1;
    }

    if (strcmp(argv[*index], "--icons") == 0) {
        uint16_t icons[ICON_COUNT];
        if (!require_value(argc, argv, index, &value)) {
            return -1;
        }
        if (!parse_icons(value, icons) || !card_set_icons(card, icons[0], icons[1], icons[2], err, sizeof(err))) {
            fputs("error: --icons must contain exactly three Gen IV Pokemon "
                  "names (or none)\n",
                  stderr);
            return -1;
        }
        return 1;
    }

    if (strcmp(argv[*index], "--received") == 0) {
        uint16_t days;
        if (!require_value(argc, argv, index, &value)) {
            return -1;
        }
        if (!parse_received_date(value, &days)) {
            fputs("error: received date must be YYYY-MM-DD on or after 2000-01-01\n", stderr);
            return -1;
        }
        card_set_received_days(card, days);
        return 1;
    }

    if (strcmp(argv[*index], "-o") == 0 || strcmp(argv[*index], "--output") == 0) {
        if (!require_value(argc, argv, index, output_path)) {
            return -1;
        }
        return 1;
    }

    if (strcmp(argv[*index], "--force") == 0) {
        *force = 1;
        return 1;
    }

    return 0;
}

static int parse_create_options(Pcd4Card *card, int argc, char **argv, CreateOptions *options) {
    int i;

    memset(options, 0, sizeof(*options));
    options->type = GIFT_NONE;

    for (i = 2; i < argc; ++i) {
        const char *value;
        int metadata = parse_metadata_option(card, argc, argv, &i, &options->output_path, &options->force);

        if (metadata < 0) {
            return 0;
        }

        if (metadata > 0) {
            continue;
        }

        if (strcmp(argv[i], "--type") == 0) {
            if (!require_value(argc, argv, &i, &value) || !parse_type(value, &options->type)) {
                fputs("error: unknown or missing gift type\n", stderr);
                return 0;
            }

            options->have_type = 1;
        } else if (strcmp(argv[i], "--pk4") == 0) {
            if (!require_value(argc, argv, &i, &options->pk4_path)) {
                return 0;
            }
        } else if (strcmp(argv[i], "--item") == 0) {
            if (!require_value(argc, argv, &i, &value) || !parse_u32(value, &options->item_id)) {
                fputs("error: invalid item ID\n", stderr);
                return 0;
            }

            options->have_item = 1;
        } else if (strcmp(argv[i], "--subtype") == 0) {
            if (!require_value(argc, argv, &i, &value) || !parse_u32(value, &options->subtype)) {
                fputs("error: invalid subtype\n", stderr);
                return 0;
            }

            options->have_subtype = 1;
        } else if (strcmp(argv[i], "--value") == 0) {
            if (!require_value(argc, argv, &i, &value) || !parse_u32(value, &options->subtype_value)) {
                fputs("error: invalid subtype value\n", stderr);
                return 0;
            }

            options->have_subtype_value = 1;
        } else if (strcmp(argv[i], "--app") == 0) {
            if (!require_value(argc, argv, &i, &value) || !parse_u32(value, &options->app_id)) {
                fputs("error: invalid Poketch app ID\n", stderr);
                return 0;
            }

            options->have_app = 1;
        } else if (strcmp(argv[i], "--course") == 0) {
            if (!require_value(argc, argv, &i, &value) || !parse_u32(value, &options->course_id)) {
                fputs("error: invalid Pokewalker course ID\n", stderr);
                return 0;
            }

            options->have_course = 1;
        } else {
            fprintf(stderr, "error: unexpected argument: %s\n", argv[i]);
            return 0;
        }
    }

    return 1;
}

static int build_gift(Pcd4Card *card, const CreateOptions *options, char *err, size_t err_size) {
    if (!options->have_type) {
        snprintf(err, err_size, "--type is required");
        return 0;
    }

    switch (options->type) {
    case GIFT_POKEMON:
    case GIFT_POKEMON_EGG: {
        Pcd4Pk4 pk4;
        if (options->pk4_path == NULL) {
            snprintf(err, err_size, "--pk4 is required for Pokemon gifts");
            return 0;
        }

        if (!pk4_load(options->pk4_path, &pk4, err, err_size)) {
            return 0;
        }

        return card_set_pk4(card, options->type, &pk4, err, err_size);
    }
    case GIFT_ITEM:
        if (!options->have_item) {
            snprintf(err, err_size, "--item is required for item gifts");
            return 0;
        }

        return card_set_item(card, options->item_id, err, err_size);
    case GIFT_SUBTYPE:
        if (!options->have_subtype || !options->have_subtype_value) {
            snprintf(err, err_size, "--subtype and --value are required");
            return 0;
        }

        return card_set_subtype(card, options->subtype, options->subtype_value, err, err_size);
    case GIFT_POKETCH_APP:
        if (!options->have_app) {
            snprintf(err, err_size, "--app is required for Poketch app gifts");
            return 0;
        }

        return card_set_poketch_app(card, options->app_id, err, err_size);
    case GIFT_POKEWALKER_COURSE:
        if (!options->have_course) {
            snprintf(err, err_size, "--course is required for Pokewalker course gifts");
            return 0;
        }

        return card_set_pokewalker_course(card, options->course_id, err, err_size);
    default:
        return card_set_type(card, options->type, err, err_size);
    }
}

static int inspect_card(const char *path, int validate_only) {
    Pcd4Card card;
    char err[ERROR_SIZE];
    char title[TITLE_BYTES * 2u];
    char description[INPUT_SIZE];
    char received[11];
    uint16_t icons[ICON_COUNT];
    Pcd4GiftType type;

    if (!card_load(path, &card, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 1;
    }

    if (!card_validate(&card, err, sizeof(err))) {
        fprintf(stderr, "invalid: %s\n", err);
        return 2;
    }

    if (validate_only) {
        puts("valid");
        return 0;
    }

    if (!card_get_title(&card, title, sizeof(title))) {
        strcpy(title, "<unreadable>");
    }

    if (!card_get_description(&card, description, sizeof(description))) {
        strcpy(description, "<unreadable>");
    }

    card_get_icons(&card, icons);
    format_received_date(card_received_days(&card), received);
    type = card_type(&card);

    printf("File: %s\n", path);
    printf("Size: %u bytes\n", (unsigned)PCD_SIZE);
    printf("Gift type: %u (%s)\n", (unsigned)type, gift_type_name(type));
    printf("Card ID: %u\n", (unsigned)card_card_id(&card));
    printf("Title: %s\n", title);
    printf("Description: %s\n", description);
    fputs("Games: ", stdout);
    print_games(card_compatibility(&card));
    putchar('\n');
    printf("Redistribution: %u\n", (unsigned)card_redistribution(&card));
    printf("Icons: %s, %s, %s\n", species_name(icons[0]), species_name(icons[1]), species_name(icons[2]));
    printf("Received: %s (day %u since 2000-01-01)\n", received, (unsigned)card_received_days(&card));

    if (type == GIFT_ITEM) {
        printf("Item ID: %u\n", (unsigned)read_le32(card.data + 4u));
    } else if (type == GIFT_POKEWALKER_COURSE) {
        printf("Course ID: %u\n", (unsigned)card.data[4u]);
    }
    return 0;
}

static int command_create(int argc, char **argv) {
    Pcd4Card card;
    CreateOptions options;
    char err[ERROR_SIZE];

    card_init(&card);
    if (!parse_create_options(&card, argc, argv, &options)) {
        return 1;
    }

    if (!build_gift(&card, &options, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 1;
    }

    if (options.output_path == NULL) {
        fputs("error: output path (-o) is required\n", stderr);
        return 1;
    }

    if (!card_save(options.output_path, &card, options.force, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 1;
    }

    printf("wrote %s\n", options.output_path);
    return 0;
}

static int command_edit(int argc, char **argv) {
    Pcd4Card card;
    const char *output_path = NULL;
    int force = 0;
    int i;
    char err[ERROR_SIZE];

    if (argc < 4) {
        usage();
        return 1;
    }

    if (!card_load(argv[2], &card, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 1;
    }

    for (i = 3; i < argc; ++i) {
        int result = parse_metadata_option(&card, argc, argv, &i, &output_path, &force);
        if (result < 0) {
            return 1;
        }

        if (result == 0) {
            fprintf(stderr, "error: unexpected edit argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (output_path == NULL) {
        fputs("error: output path (-o) is required\n", stderr);
        return 1;
    }

    if (!card_save(output_path, &card, force, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 1;
    }

    printf("wrote %s\n", output_path);
    return 0;
}

static void trim_newline(char *text) {
    size_t length = strlen(text);
    while (length > 0u && (text[length - 1u] == '\n' || text[length - 1u] == '\r')) {
        text[--length] = '\0';
    }
}

static int prompt_line(const char *label, char *buffer, size_t size) {
    fputs(label, stdout);
    fflush(stdout);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    trim_newline(buffer);
    return 1;
}

static int prompt_u32(const char *label, uint32_t *value) {
    char line[INPUT_SIZE];
    return prompt_line(label, line, sizeof(line)) && parse_u32(line, value);
}

static int interactive_create_one(void) {
    Pcd4Card card;
    Pcd4GiftType type;
    uint32_t value;
    uint16_t games;
    char line[INPUT_SIZE];
    char path[INPUT_SIZE];
    char err[ERROR_SIZE];

    card_init(&card);
    puts("\nGift types:\n"
         "  1  Pokemon\n"
         "  2  Pokemon Egg\n"
         "  3  Item\n"
         "  6  Seal/Accessory/Backdrop\n"
         "  7  Manaphy Egg\n"
         "  8  Member Card\n"
         "  9  Oak's Letter\n"
         " 10  Azure Flute\n"
         " 11  Poketch App\n"
         " 12  Secret Key\n"
         " 14  Pokewalker Course");

    if (!prompt_u32("Gift type number: ", &value)) {
        fputs("error: invalid gift type\n", stderr);
        return 0;
    }

    type = (Pcd4GiftType)value;

    if (type == GIFT_POKEMON || type == GIFT_POKEMON_EGG) {
        Pcd4Pk4 pk4;

        if (!prompt_line("PK4 path: ", path, sizeof(path))) {
            return 0;
        }

        if (!pk4_load(path, &pk4, err, sizeof(err)) || !card_set_pk4(&card, type, &pk4, err, sizeof(err))) {
            fprintf(stderr, "error: %s\n", err);
            return 0;
        }
    } else if (type == GIFT_ITEM) {
        if (!prompt_u32("Item ID (1..536): ", &value)) {
            fputs("error: invalid item ID\n", stderr);
            return 0;
        }
        if (!card_set_item(&card, value, err, sizeof(err))) {
            fprintf(stderr, "error: %s\n", err);
            return 0;
        }
    } else if (type == GIFT_SUBTYPE) {
        uint32_t subtype;
        uint32_t subtype_value;

        if (!prompt_u32("Subtype (1 Seal, 2 Accessory, 3 Backdrop): ", &subtype)) {
            fputs("error: invalid subtype\n", stderr);
            return 0;
        }
        if (!prompt_u32("Subtype value ID: ", &subtype_value)) {
            fputs("error: invalid subtype value\n", stderr);
            return 0;
        }
        if (!card_set_subtype(&card, subtype, subtype_value, err, sizeof(err))) {
            fprintf(stderr, "error: %s\n", err);
            return 0;
        }
    } else if (type == GIFT_POKETCH_APP) {
        if (!prompt_u32("Poketch app ID: ", &value)) {
            fputs("error: invalid Poketch app ID\n", stderr);
            return 0;
        }
        if (!card_set_poketch_app(&card, value, err, sizeof(err))) {
            fprintf(stderr, "error: %s\n", err);
            return 0;
        }
    } else if (type == GIFT_POKEWALKER_COURSE) {
        if (!prompt_u32("Pokewalker course ID: ", &value)) {
            fputs("error: invalid Pokewalker course ID\n", stderr);
            return 0;
        }
        if (!card_set_pokewalker_course(&card, value, err, sizeof(err))) {
            fprintf(stderr, "error: %s\n", err);
            return 0;
        }
    } else if (!card_set_type(&card, type, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 0;
    }

    if (!prompt_line("Card title (max 35 chars): ", line, sizeof(line))) {
        return 0;
    }
    if (!card_set_title(&card, line, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 0;
    }

    if (!prompt_u32("Card ID (0..65535): ", &value) || value > 65535u) {
        fputs("error: invalid card ID\n", stderr);
        return 0;
    }
    card_set_card_id(&card, (uint16_t)value);

    if (!prompt_line("Compatible games (d,p,pt,hg,ss): ", line, sizeof(line)) || !parse_games(line, &games)) {
        fputs("error: invalid game list\n", stderr);
        return 0;
    }

    card_set_compatibility(&card, games);

    if (!prompt_line("Description (optional): ", line, sizeof(line))) {
        return 0;
    }
    if (!card_set_description(&card, line, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 0;
    }

    if (!prompt_u32("Redistribution byte (0..255): ", &value) || value > 255u) {
        fputs("error: invalid redistribution limit\n", stderr);
        return 0;
    }

    card_set_redistribution(&card, (uint8_t)value);

    {
        uint16_t icons[ICON_COUNT];
        if (!prompt_icon("Left card icon Pokemon (name or none): ", &icons[0]) || !prompt_icon("Middle card icon Pokemon (name or none): ", &icons[1]) ||
            !prompt_icon("Right card icon Pokemon (name or none): ", &icons[2]) || !card_set_icons(&card, icons[0], icons[1], icons[2], err, sizeof(err))) {
            fputs("error: invalid icon list\n", stderr);
            return 0;
        }
    }

    if (!prompt_line("Received date (YYYY-MM-DD, 2000-01-01 for day 0): ", line, sizeof(line))) {
        return 0;
    }
    {
        uint16_t received_days;
        if (!parse_received_date(line, &received_days)) {
            fputs("error: invalid received date\n", stderr);
            return 0;
        }
        card_set_received_days(&card, received_days);
    }

    if (!prompt_line("Output .pcd path: ", path, sizeof(path))) {
        return 0;
    }

    if (!card_save(path, &card, 0, err, sizeof(err))) {
        fprintf(stderr, "error: %s\n", err);
        return 0;
    }

    printf("wrote %s\n", path);
    return 1;
}

static int interactive(void) {
    char answer[32];

    puts("PCD4 interactive creator");
    for (;;) {
        (void)interactive_create_one();

        if (!prompt_line("Create a new gift without restarting? [y/N]: ", answer, sizeof(answer))) {
            return 0;
        }

        if (!str_ieq(answer, "y") && !str_ieq(answer, "yes")) {
            return 0;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 1;
    }

    if (strcmp(argv[1], "create") == 0) {
        return command_create(argc, argv);
    }

    if (strcmp(argv[1], "edit") == 0) {
        return command_edit(argc, argv);
    }

    if (strcmp(argv[1], "inspect") == 0 && argc == 3) {
        return inspect_card(argv[2], 0);
    }

    if (strcmp(argv[1], "validate") == 0 && argc == 3) {
        return inspect_card(argv[2], 1);
    }

    if (strcmp(argv[1], "interactive") == 0) {
        return interactive();
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage();
        return 0;
    }

    usage();
    return 1;
}
