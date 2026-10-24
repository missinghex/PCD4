#include "species.h"

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define SPECIES_MAX 493u
#define NORMALIZED_NAME_SIZE 32u

static const char *const species_names[SPECIES_MAX + 1u] = {
    "None",       "Bulbasaur",  "Ivysaur",    "Venusaur",   "Charmander", "Charmeleon", "Charizard",  "Squirtle",  "Wartortle", "Blastoise",  "Caterpie",   "Metapod",    "Butterfree", "Weedle",
    "Kakuna",     "Beedrill",   "Pidgey",     "Pidgeotto",  "Pidgeot",    "Rattata",    "Raticate",   "Spearow",   "Fearow",    "Ekans",      "Arbok",      "Pikachu",    "Raichu",     "Sandshrew",
    "Sandslash",  "Nidoran F",  "Nidorina",   "Nidoqueen",  "Nidoran M",  "Nidorino",   "Nidoking",   "Clefairy",  "Clefable",  "Vulpix",     "Ninetales",  "Jigglypuff", "Wigglytuff", "Zubat",
    "Golbat",     "Oddish",     "Gloom",      "Vileplume",  "Paras",      "Parasect",   "Venonat",    "Venomoth",  "Diglett",   "Dugtrio",    "Meowth",     "Persian",    "Psyduck",    "Golduck",
    "Mankey",     "Primeape",   "Growlithe",  "Arcanine",   "Poliwag",    "Poliwhirl",  "Poliwrath",  "Abra",      "Kadabra",   "Alakazam",   "Machop",     "Machoke",    "Machamp",    "Bellsprout",
    "Weepinbell", "Victreebel", "Tentacool",  "Tentacruel", "Geodude",    "Graveler",   "Golem",      "Ponyta",    "Rapidash",  "Slowpoke",   "Slowbro",    "Magnemite",  "Magneton",   "Farfetch'd",
    "Doduo",      "Dodrio",     "Seel",       "Dewgong",    "Grimer",     "Muk",        "Shellder",   "Cloyster",  "Gastly",    "Haunter",    "Gengar",     "Onix",       "Drowzee",    "Hypno",
    "Krabby",     "Kingler",    "Voltorb",    "Electrode",  "Exeggcute",  "Exeggutor",  "Cubone",     "Marowak",   "Hitmonlee", "Hitmonchan", "Lickitung",  "Koffing",    "Weezing",    "Rhyhorn",
    "Rhydon",     "Chansey",    "Tangela",    "Kangaskhan", "Horsea",     "Seadra",     "Goldeen",    "Seaking",   "Staryu",    "Starmie",    "Mr. Mime",   "Scyther",    "Jynx",       "Electabuzz",
    "Magmar",     "Pinsir",     "Tauros",     "Magikarp",   "Gyarados",   "Lapras",     "Ditto",      "Eevee",     "Vaporeon",  "Jolteon",    "Flareon",    "Porygon",    "Omanyte",    "Omastar",
    "Kabuto",     "Kabutops",   "Aerodactyl", "Snorlax",    "Articuno",   "Zapdos",     "Moltres",    "Dratini",   "Dragonair", "Dragonite",  "Mewtwo",     "Mew",        "Chikorita",  "Bayleef",
    "Meganium",   "Cyndaquil",  "Quilava",    "Typhlosion", "Totodile",   "Croconaw",   "Feraligatr", "Sentret",   "Furret",    "Hoothoot",   "Noctowl",    "Ledyba",     "Ledian",     "Spinarak",
    "Ariados",    "Crobat",     "Chinchou",   "Lanturn",    "Pichu",      "Cleffa",     "Igglybuff",  "Togepi",    "Togetic",   "Natu",       "Xatu",       "Mareep",     "Flaaffy",    "Ampharos",
    "Bellossom",  "Marill",     "Azumarill",  "Sudowoodo",  "Politoed",   "Hoppip",     "Skiploom",   "Jumpluff",  "Aipom",     "Sunkern",    "Sunflora",   "Yanma",      "Wooper",     "Quagsire",
    "Espeon",     "Umbreon",    "Murkrow",    "Slowking",   "Misdreavus", "Unown",      "Wobbuffet",  "Girafarig", "Pineco",    "Forretress", "Dunsparce",  "Gligar",     "Steelix",    "Snubbull",
    "Granbull",   "Qwilfish",   "Scizor",     "Shuckle",    "Heracross",  "Sneasel",    "Teddiursa",  "Ursaring",  "Slugma",    "Magcargo",   "Swinub",     "Piloswine",  "Corsola",    "Remoraid",
    "Octillery",  "Delibird",   "Mantine",    "Skarmory",   "Houndour",   "Houndoom",   "Kingdra",    "Phanpy",    "Donphan",   "Porygon2",   "Stantler",   "Smeargle",   "Tyrogue",    "Hitmontop",
    "Smoochum",   "Elekid",     "Magby",      "Miltank",    "Blissey",    "Raikou",     "Entei",      "Suicune",   "Larvitar",  "Pupitar",    "Tyranitar",  "Lugia",      "Ho-Oh",      "Celebi",
    "Treecko",    "Grovyle",    "Sceptile",   "Torchic",    "Combusken",  "Blaziken",   "Mudkip",     "Marshtomp", "Swampert",  "Poochyena",  "Mightyena",  "Zigzagoon",  "Linoone",    "Wurmple",
    "Silcoon",    "Beautifly",  "Cascoon",    "Dustox",     "Lotad",      "Lombre",     "Ludicolo",   "Seedot",    "Nuzleaf",   "Shiftry",    "Taillow",    "Swellow",    "Wingull",    "Pelipper",
    "Ralts",      "Kirlia",     "Gardevoir",  "Surskit",    "Masquerain", "Shroomish",  "Breloom",    "Slakoth",   "Vigoroth",  "Slaking",    "Nincada",    "Ninjask",    "Shedinja",   "Whismur",
    "Loudred",    "Exploud",    "Makuhita",   "Hariyama",   "Azurill",    "Nosepass",   "Skitty",     "Delcatty",  "Sableye",   "Mawile",     "Aron",       "Lairon",     "Aggron",     "Meditite",
    "Medicham",   "Electrike",  "Manectric",  "Plusle",     "Minun",      "Volbeat",    "Illumise",   "Roselia",   "Gulpin",    "Swalot",     "Carvanha",   "Sharpedo",   "Wailmer",    "Wailord",
    "Numel",      "Camerupt",   "Torkoal",    "Spoink",     "Grumpig",    "Spinda",     "Trapinch",   "Vibrava",   "Flygon",    "Cacnea",     "Cacturne",   "Swablu",     "Altaria",    "Zangoose",
    "Seviper",    "Lunatone",   "Solrock",    "Barboach",   "Whiscash",   "Corphish",   "Crawdaunt",  "Baltoy",    "Claydol",   "Lileep",     "Cradily",    "Anorith",    "Armaldo",    "Feebas",
    "Milotic",    "Castform",   "Kecleon",    "Shuppet",    "Banette",    "Duskull",    "Dusclops",   "Tropius",   "Chimecho",  "Absol",      "Wynaut",     "Snorunt",    "Glalie",     "Spheal",
    "Sealeo",     "Walrein",    "Clamperl",   "Huntail",    "Gorebyss",   "Relicanth",  "Luvdisc",    "Bagon",     "Shelgon",   "Salamence",  "Beldum",     "Metang",     "Metagross",  "Regirock",
    "Regice",     "Registeel",  "Latias",     "Latios",     "Kyogre",     "Groudon",    "Rayquaza",   "Jirachi",   "Deoxys",    "Turtwig",    "Grotle",     "Torterra",   "Chimchar",   "Monferno",
    "Infernape",  "Piplup",     "Prinplup",   "Empoleon",   "Starly",     "Staravia",   "Staraptor",  "Bidoof",    "Bibarel",   "Kricketot",  "Kricketune", "Shinx",      "Luxio",      "Luxray",
    "Budew",      "Roserade",   "Cranidos",   "Rampardos",  "Shieldon",   "Bastiodon",  "Burmy",      "Wormadam",  "Mothim",    "Combee",     "Vespiquen",  "Pachirisu",  "Buizel",     "Floatzel",
    "Cherubi",    "Cherrim",    "Shellos",    "Gastrodon",  "Ambipom",    "Drifloon",   "Drifblim",   "Buneary",   "Lopunny",   "Mismagius",  "Honchkrow",  "Glameow",    "Purugly",    "Chingling",
    "Stunky",     "Skuntank",   "Bronzor",    "Bronzong",   "Bonsly",     "Mime Jr.",   "Happiny",    "Chatot",    "Spiritomb", "Gible",      "Gabite",     "Garchomp",   "Munchlax",   "Riolu",
    "Lucario",    "Hippopotas", "Hippowdon",  "Skorupi",    "Drapion",    "Croagunk",   "Toxicroak",  "Carnivine", "Finneon",   "Lumineon",   "Mantyke",    "Snover",     "Abomasnow",  "Weavile",
    "Magnezone",  "Lickilicky", "Rhyperior",  "Tangrowth",  "Electivire", "Magmortar",  "Togekiss",   "Yanmega",   "Leafeon",   "Glaceon",    "Gliscor",    "Mamoswine",  "Porygon-Z",  "Gallade",
    "Probopass",  "Dusknoir",   "Froslass",   "Rotom",      "Uxie",       "Mesprit",    "Azelf",      "Dialga",    "Palkia",    "Heatran",    "Regigigas",  "Giratina",   "Cresselia",  "Phione",
    "Manaphy",    "Darkrai",    "Shaymin",    "Arceus"};

static int normalize_name(const char *text, char *out, size_t out_size) {
    size_t written = 0u;

    if (text == NULL || out == NULL || out_size == 0u) {
        return 0;
    }

    while (*text != '\0') {
        unsigned char ch = (unsigned char)*text++;
        if (isalnum(ch)) {
            if (written + 1u >= out_size) {
                return 0;
            }

            out[written++] = (char)tolower(ch);
        } else if (!isspace(ch) && ch != '-' && ch != '.' && ch != '\'') {
            return 0;
        }
    }

    out[written] = '\0';
    return written != 0u;
}

const char *species_name(uint16_t species_id) {
    if (species_id > SPECIES_MAX) {
        return NULL;
    }
    return species_names[species_id];
}

int species_parse(const char *text, uint16_t *species_id) {
    char normalized[NORMALIZED_NAME_SIZE];
    char candidate[NORMALIZED_NAME_SIZE];
    char *end = NULL;
    unsigned long numeric;
    uint16_t id;

    if (text == NULL || species_id == NULL) {
        return 0;
    }

    while (isspace((unsigned char)*text)) {
        ++text;
    }

    errno = 0;
    numeric = strtoul(text, &end, 10);
    if (errno == 0 && end != text) {
        while (isspace((unsigned char)*end)) {
            ++end;
        }
        if (*end == '\0' && numeric <= SPECIES_MAX) {
            *species_id = (uint16_t)numeric;
            return 1;
        }
    }

    if (!normalize_name(text, normalized, sizeof(normalized))) {
        return 0;
    }

    if (strcmp(normalized, "none") == 0 || strcmp(normalized, "blank") == 0) {
        *species_id = 0u;
        return 1;
    }

    for (id = 1u; id <= SPECIES_MAX; ++id) {
        if (!normalize_name(species_names[id], candidate, sizeof(candidate))) {
            return 0;
        }
        if (strcmp(normalized, candidate) == 0) {
            *species_id = id;
            return 1;
        }
    }

    return 0;
}
