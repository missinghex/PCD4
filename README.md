# PCD4

PCD4 is a portable command-line tool for creating, inspecting, validating, and editing Pokemon Generation IV Mystery Gift `.pcd` files.

## Features

- Create Generation IV Mystery Gift `.pcd` files from the command line
- Interactive card creation
- Inspect existing `.pcd` files
- Validate card data
- Edit Mystery Gift metadata
- Create Pokemon gifts from `.pk4` files
- Select compatible Generation IV games
- Set Wonder Card title and description text
- Select Pokemon icons by species name or National Pokedex number

## Mystery Gift Types

| ID | Gift Type |
| ---: | --- |
| 1 | Pokémon |
| 2 | Pokémon Egg |
| 3 | Item |
| 6 | Seal / Accessory / Backdrop |
| 7 | Manaphy Egg |
| 8 | Member Card |
| 9 | Oak's Letter |
| 10 | Azure Flute |
| 11 | Pokétch App |
| 12 | Secret Key |
| 14 | Pokéwalker Course |

The following gift types are recognized but are not yet fully implemented:

| ID | Gift Type |
| ---: | --- |
| 4 | Rule |
| 5 | Goods |
| 13 | Pokémon Movie |
| 15 | Memorial Photo |

## Supported Games

| ID | Game |
| --- | --- |
| `d` | Pokémon Diamond |
| `p` | Pokémon Pearl |
| `pt` | Pokémon Platinum |
| `hg` | Pokémon HeartGold |
| `ss` | Pokémon SoulSilver |

## Prerequisites

PCD4 requires:

- A C11-compatible compiler such as GCC or Clang
- CMake 3.16 or newer
- A CMake-supported build tool such as Ninja or Make

## Building

Configure and build the project:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run the test suite:

```sh
ctest --test-dir build --output-on-failure
```

The resulting executable is:

```text
build/pcd4
```

## Usage

Display the available commands and options:

```sh
./build/pcd4 --help
```

### Create a Member Card

The following example creates a Member Card compatible with Pokemon Platinum:

```sh
./build/pcd4 create \
  --type member-card \
  --title "Member Card" \
  --description "For more information on how to get DARKRAI,\nvisit the official Pokémon website.\nBe sure to save your game after you\npick up the Member Card at a\nPoké Mart." \
  --card-id 18 \
  --games pt \
  --redistribution 0 \
  --icons none,darkrai,none \
  --received 2009-08-03 \
  -o membercard.pcd
```

PCD4 interprets `\n` in description text as a manual line break.

### Create a Pokemon Gift

A Pokemon gift can be created directly from a Generation IV `.pk4` file:

```sh
./build/pcd4 create \
  --type pokemon \
  --pk4 event.pk4 \
  --title "A special Pokémon!" \
  --card-id 62 \
  --games d,p,pt,hg,ss \
  -o gift.pcd
```

### Interactive Creation

PCD4 also includes an interactive creator:

```sh
./build/pcd4 interactive
```

The interactive mode guides you through the card fields instead of requiring every option on the command line.

## Inspecting and Validating Cards

Inspect the contents of an existing `.pcd` file:

```sh
./build/pcd4 inspect gift.pcd
```

Validate a card:

```sh
./build/pcd4 validate gift.pcd
```

## Editing Existing Cards

Card metadata can be changed without recreating the card from scratch:

```sh
./build/pcd4 edit existing.pcd \
  --title "Edited Card" \
  --description "Updated text." \
  --games pt,hg,ss \
  --icons darkrai,none,dialga \
  -o edited.pcd
```

## Pokemon Icons

The `--icons` option accepts three comma-separated Generation IV Pokemon species representing the left, middle, and right icon positions:

```text
--icons left,middle,right
```

Use `none` for an empty slot:

```sh
--icons none,darkrai,none
```

Species names are matched case-insensitively. Normal spaces, hyphens, periods, and apostrophes are ignored during matching.

For example, all of the following resolve to the same species:

```text
Mr. Mime
mr mime
mr-mime
```

National Pokedex IDs from `0` through `493` are also accepted.

In interactive mode, PCD4 asks for the left, middle, and right icons separately.

## Output Safety

PCD4 refuses to overwrite an existing output file by default.

To intentionally replace an existing file, use:

```sh
--force
```

Example:

```sh
./build/pcd4 create \
  --type member-card \
  --title "Member Card" \
  -o membercard.pcd \
  --force
```

This helps prevent existing Mystery Gift files from being overwritten accidentally.