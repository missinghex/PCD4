#include "text.h"
#include "common.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct TextCharMap {
    uint32_t unicode;
    uint16_t code;
} TextCharMap;

/*
 * Explicit DS character mappings.
 */
static const TextCharMap text_char_map[] = {
    {0x0021u, 0x01ABu},  /* ! */
    {0x003Fu, 0x01ACu},  /* ? */
    {0x002Cu, 0x01ADu},  /* , */
    {0x002Eu, 0x01AEu},  /* . */
    {0x002Fu, 0x01B1u},  /* / */
    {0x0027u, 0x01B3u},  /* ' */
    {0x0028u, 0x01B9u},  /* ( */
    {0x0029u, 0x01BAu},  /* ) */
    {0x002Bu, 0x01BDu},  /* + */
    {0x002Du, 0x01BEu},  /* - */
    {0x002Au, 0x01BFu},  /* * */
    {0x0023u, 0x01C0u},  /* # */
    {0x003Du, 0x01C1u},  /* = */
    {0x0026u, 0x01C2u},  /* & */
    {0x007Eu, 0x01C3u},  /* ~ */
    {0x003Au, 0x01C4u},  /* : */
    {0x003Bu, 0x01C5u},  /* ; */
    {0x0040u, 0x01D0u},  /* @ */
    {0x0025u, 0x01D2u},  /* % */
    {0x005Fu, 0x01E9u},  /* _ */
    {0x00C0u, 0x015Fu},  /* À */
    {0x00C1u, 0x0160u},  /* Á */
    {0x00C4u, 0x0163u},  /* Ä */
    {0x00C7u, 0x0166u},  /* Ç */
    {0x00C8u, 0x0167u},  /* È */
    {0x00C9u, 0x0168u},  /* É */
    {0x00CCu, 0x016Bu},  /* Ì */
    {0x00CDu, 0x016Cu},  /* Í */
    {0x00D1u, 0x0170u},  /* Ñ */
    {0x00D2u, 0x0171u},  /* Ò */
    {0x00D3u, 0x0172u},  /* Ó */
    {0x00D6u, 0x0175u},  /* Ö */
    {0x00D9u, 0x0178u},  /* Ù */
    {0x00DAu, 0x0179u},  /* Ú */
    {0x00DCu, 0x017Bu},  /* Ü */
    {0x00DFu, 0x017Eu},  /* ß */
    {0x00E0u, 0x017Fu},  /* à */
    {0x00E1u, 0x0180u},  /* á */
    {0x00E2u, 0x0181u},  /* â */
    {0x00E4u, 0x0183u},  /* ä */
    {0x00E7u, 0x0186u},  /* ç */
    {0x00E8u, 0x0187u},  /* è */
    {0x00E9u, 0x0188u},  /* é */
    {0x00EAu, 0x0189u},  /* ê */
    {0x00EBu, 0x018Au},  /* ë */
    {0x00ECu, 0x018Bu},  /* ì */
    {0x00EDu, 0x018Cu},  /* í */
    {0x00EEu, 0x018Du},  /* î */
    {0x00EFu, 0x018Eu},  /* ï */
    {0x00F1u, 0x0190u},  /* ñ */
    {0x00F2u, 0x0191u},  /* ò */
    {0x00F3u, 0x0192u},  /* ó */
    {0x00F4u, 0x0193u},  /* ô */
    {0x00F6u, 0x0195u},  /* ö */
    {0x00F9u, 0x0198u},  /* ù */
    {0x00FAu, 0x0199u},  /* ú */
    {0x00FBu, 0x019Au},  /* û */
    {0x00FCu, 0x019Bu},  /* ü */
    {0x00A1u, 0x01A9u},  /* ¡ */
    {0x00BFu, 0x01AAu},  /* ¿ */
    {0x2026u, 0x01AFu},  /* … */
    {0xFF65u, 0x01B0u},  /* ･ */
    {0x2018u, 0x01B2u},  /* ‘ */
    {0x201Cu, 0x01B4u},  /* “ */
    {0x201Du, 0x01B5u},  /* ” */
    {0x201Eu, 0x01B6u},  /* „ */
    {0x00ABu, 0x01B7u},  /* « */
    {0x00BBu, 0x01B8u},  /* » */
    {0x2642u, 0x01BBu},  /* ♂ */
    {0x2640u, 0x01BCu},  /* ♀ */
    {0x2660u, 0x01C6u},  /* ♠ */
    {0x2663u, 0x01C7u},  /* ♣ */
    {0x2665u, 0x01C8u},  /* ♥ */
    {0x2666u, 0x01C9u},  /* ♦ */
    {0x2605u, 0x01CAu},  /* ★ */
    {0x25CEu, 0x01CBu},  /* ◎ */
    {0x25CBu, 0x01CCu},  /* ○ */
    {0x25A1u, 0x01CDu},  /* □ */
    {0x25B3u, 0x01CEu},  /* △ */
    {0x25C7u, 0x01CFu},  /* ◇ */
    {0x266Au, 0x01D1u},  /* ♪ */
    {0x2600u, 0x01D3u},  /* ☀ */
    {0x2601u, 0x01D4u},  /* ☁ */
    {0x2602u, 0x01D5u},  /* ☂ */
    {0x2603u, 0x01D6u},  /* ☃ */
    {0x1F642u, 0x01D7u}, /* 🙂; DS regular-face placeholder */
    {0x1F603u, 0x01D8u}, /* 😃; DS happy-face placeholder */
    {0x1F62Bu, 0x01D9u}, /* 😫; DS sad-face placeholder */
    {0x1F620u, 0x01DAu}, /* 😠; DS mad-face placeholder */
    {0x2191u, 0x01DBu},  /* ↑ */
    {0x2193u, 0x01DCu},  /* ↓ */
    {0x1F634u, 0x01DDu}, /* 😴; DS double-Z placeholder */
};

static int encode_char(uint32_t c, uint16_t *code) {
    size_t i;

    if (c == 0x000Au) { /* newline */
        *code = 0xE000u;
        return 1;
    }

    if (c == 0x0020u) { /* space */
        *code = 0x01DEu;
        return 1;
    }

    if (c >= 0x0030u && c <= 0x0039u) { /* 0-9 */
        *code = (uint16_t)(0x0121u + (uint16_t)(c - 0x0030u));
        return 1;
    }

    if (c >= 0x0041u && c <= 0x005Au) { /* A-Z */
        *code = (uint16_t)(0x012Bu + (uint16_t)(c - 0x0041u));
        return 1;
    }

    if (c >= 0x0061u && c <= 0x007Au) { /* a-z */
        *code = (uint16_t)(0x0145u + (uint16_t)(c - 0x0061u));
        return 1;
    }

    for (i = 0; i < sizeof(text_char_map) / sizeof(text_char_map[0]); ++i) {
        if (text_char_map[i].unicode == c) {
            *code = text_char_map[i].code;
            return 1;
        }
    }

    return 0;
}

static uint32_t decode_char(uint16_t code) {
    size_t i;

    if (code == 0xE000u)
        return 0x000Au; /* newline */

    if (code == 0x01DEu)
        return 0x0020u; /* space */

    if (code >= 0x0121u && code <= 0x012Au)
        return 0x0030u + (uint32_t)(code - 0x0121u); /* 0-9 */

    if (code >= 0x012Bu && code <= 0x0144u)
        return 0x0041u + (uint32_t)(code - 0x012Bu); /* A-Z */

    if (code >= 0x0145u && code <= 0x015Eu)
        return 0x0061u + (uint32_t)(code - 0x0145u); /* a-z */

    for (i = 0; i < sizeof(text_char_map) / sizeof(text_char_map[0]); ++i) {
        if (text_char_map[i].code == code)
            return text_char_map[i].unicode;
    }

    return 0x003Fu; /* ? */
}

/*
 * Read one Unicode scalar value from a UTF-8 byte string.
 * Returns 1 on success and advances *pos, otherwise returns 0.
 */
static int utf8_next(const char *src, size_t len, size_t *pos, uint32_t *cp) {
    const unsigned char *s = (const unsigned char *)src;
    size_t p = *pos;
    unsigned char b0;
    unsigned char b1;
    unsigned char b2;
    unsigned char b3;

    if (p >= len)
        return 0;

    b0 = s[p];

    if (b0 <= 0x7Fu) {
        *cp = (uint32_t)b0;
        *pos = p + 1u;
        return 1;
    }

    if (b0 >= 0xC2u && b0 <= 0xDFu) {
        if (p + 1u >= len)
            return 0;

        b1 = s[p + 1u];
        if ((b1 & 0xC0u) != 0x80u)
            return 0;

        *cp = ((uint32_t)(b0 & 0x1Fu) << 6) | (uint32_t)(b1 & 0x3Fu);
        *pos = p + 2u;

        return 1;
    }

    if (b0 >= 0xE0u && b0 <= 0xEFu) {
        if (p + 2u >= len)
            return 0;
        b1 = s[p + 1u];
        b2 = s[p + 2u];

        if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u)
            return 0;

        /* Reject overlong encodings and UTF-16 surrogate code points. */
        if (b0 == 0xE0u && b1 < 0xA0u)
            return 0;

        if (b0 == 0xEDu && b1 > 0x9Fu)
            return 0;

        *cp = ((uint32_t)(b0 & 0x0Fu) << 12) | ((uint32_t)(b1 & 0x3Fu) << 6) | (uint32_t)(b2 & 0x3Fu);
        *pos = p + 3u;
        return 1;
    }

    if (b0 >= 0xF0u && b0 <= 0xF4u) {
        if (p + 3u >= len)
            return 0;

        b1 = s[p + 1u];
        b2 = s[p + 2u];
        b3 = s[p + 3u];

        if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u || (b3 & 0xC0u) != 0x80u)
            return 0;

        /* Reject overlong values and values above U+10FFFF. */
        if (b0 == 0xF0u && b1 < 0x90u)
            return 0;

        if (b0 == 0xF4u && b1 > 0x8Fu)
            return 0;

        *cp = ((uint32_t)(b0 & 0x07u) << 18) | ((uint32_t)(b1 & 0x3Fu) << 12) | ((uint32_t)(b2 & 0x3Fu) << 6) | (uint32_t)(b3 & 0x3Fu);
        *pos = p + 4u;

        return 1;
    }

    return 0;
}

/*
 * Append one Unicode scalar value to a UTF-8 output buffer.
 * Space for the final NUL byte is always reserved.
 */
static int utf8_append(uint32_t cp, char *dst, size_t dst_size, size_t *out) {
    size_t p = *out;

    if (cp <= 0x7Fu) {
        if (p + 1u >= dst_size)
            return 0;

        dst[p++] = (char)cp;
    } else if (cp <= 0x07FFu) {
        if (p + 2u >= dst_size)
            return 0;

        dst[p++] = (char)(0xC0u | (cp >> 6));
        dst[p++] = (char)(0x80u | (cp & 0x3Fu));
    } else if (cp <= 0xFFFFu) {
        if (cp >= 0xD800u && cp <= 0xDFFFu)
            return 0;

        if (p + 3u >= dst_size)
            return 0;

        dst[p++] = (char)(0xE0u | (cp >> 12));
        dst[p++] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        dst[p++] = (char)(0x80u | (cp & 0x3Fu));
    } else if (cp <= 0x10FFFFu) {
        if (p + 4u >= dst_size)
            return 0;

        dst[p++] = (char)(0xF0u | (cp >> 18));
        dst[p++] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        dst[p++] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        dst[p++] = (char)(0x80u | (cp & 0x3Fu));
    } else {
        return 0;
    }

    *out = p;
    return 1;
}

int text_encode(const char *src, uint8_t *dst, size_t dst_bytes, char *err, size_t err_size) {
    size_t capacity;
    size_t len;
    size_t in = 0;
    size_t out = 0;

    if (src == NULL || dst == NULL || (dst_bytes & 1u) != 0u || dst_bytes < 2u)
        return 0;

    capacity = dst_bytes / 2u;
    len = strlen(src);
    memset(dst, 0xFF, dst_bytes);

    while (in < len) {
        size_t char_byte = in;
        uint32_t cp;
        uint16_t code;

        /* Treat the two-byte escape sequence "\\n" as a line break. */
        if (src[in] == '\\' && in + 1u < len && src[in + 1u] == 'n') {
            cp = 0x000Au;
            in += 2u;
        } else if (!utf8_next(src, len, &in, &cp)) {
            if (err && err_size)
                (void)snprintf(err, err_size, "invalid UTF-8 at byte %zu", char_byte);

            return 0;
        }

        if (!encode_char(cp, &code)) {
            if (err && err_size)
                (void)snprintf(err, err_size, "unsupported text character U+%04X at byte %zu", (unsigned int)cp, char_byte);

            return 0;
        }

        /* Reserve one 16-bit slot for the 0xFFFF terminator. */
        if (out + 1u >= capacity) {
            if (err && err_size)
                (void)snprintf(err, err_size, "text is too long: maximum %zu characters", capacity - 1u);

            return 0;
        }

        write_le16(dst + (out * 2u), code);
        ++out;
    }

    write_le16(dst + (out * 2u), 0xFFFFu);
    return 1;
}

int text_decode(const uint8_t *src, size_t src_bytes, char *dst, size_t dst_size) {
    size_t i;
    size_t out = 0;

    if (!src || !dst || dst_size == 0u || (src_bytes & 1u) != 0u)
        return 0;

    for (i = 0; i + 1u < src_bytes; i += 2u) {
        uint16_t code = read_le16(src + i);
        uint32_t cp;

        if (code == 0xFFFFu)
            break;

        cp = decode_char(code);
        if (!utf8_append(cp, dst, dst_size, &out))
            return 0;
    }

    dst[out] = '\0';
    return 1;
}
