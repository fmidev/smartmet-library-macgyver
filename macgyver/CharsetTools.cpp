// ======================================================================
/*!
 * \brief Character set conversion tools
 */
// ======================================================================

#include "CharsetTools.h"
#include <algorithm>

using namespace std;

namespace Fmi
{
// ----------------------------------------------------------------------
/*!
 * \brief ISO-8859-1 lower case conversion table
 */
// ----------------------------------------------------------------------

static const unsigned char iso_8859_1_lower[256] = {0,
                                                    1,
                                                    2,
                                                    3,
                                                    4,
                                                    5,
                                                    6,
                                                    7,
                                                    8,
                                                    9,
                                                    10,
                                                    11,
                                                    12,
                                                    13,
                                                    14,
                                                    15,
                                                    16,
                                                    17,
                                                    18,
                                                    19,
                                                    20,
                                                    21,
                                                    22,
                                                    23,
                                                    24,
                                                    25,
                                                    26,
                                                    27,
                                                    28,
                                                    29,
                                                    30,
                                                    31,
                                                    32,
                                                    33,
                                                    34,
                                                    35,
                                                    36,
                                                    37,
                                                    38,
                                                    39,
                                                    40,
                                                    41,
                                                    42,
                                                    43,
                                                    44,
                                                    45,
                                                    46,
                                                    47,
                                                    48,
                                                    49,
                                                    50,
                                                    51,
                                                    52,
                                                    53,
                                                    54,
                                                    55,
                                                    56,
                                                    57,
                                                    58,
                                                    59,
                                                    60,
                                                    61,
                                                    62,
                                                    63,
                                                    64,
                                                    97,
                                                    98,
                                                    99,
                                                    100,
                                                    101,  // 65-90 = A-Z --> 97-122
                                                    102,
                                                    103,
                                                    104,
                                                    105,
                                                    106,
                                                    107,
                                                    108,
                                                    109,
                                                    110,
                                                    111,
                                                    112,
                                                    113,
                                                    114,
                                                    115,
                                                    116,
                                                    117,
                                                    118,
                                                    119,
                                                    120,
                                                    121,
                                                    122,
                                                    91,
                                                    92,
                                                    93,
                                                    94,
                                                    95,
                                                    96,
                                                    97,
                                                    98,
                                                    99,
                                                    100,
                                                    101,
                                                    102,
                                                    103,
                                                    104,
                                                    105,
                                                    106,
                                                    107,
                                                    108,
                                                    109,
                                                    110,
                                                    111,
                                                    112,
                                                    113,
                                                    114,
                                                    115,
                                                    116,
                                                    117,
                                                    118,
                                                    119,
                                                    120,
                                                    121,
                                                    122,
                                                    123,
                                                    124,
                                                    125,
                                                    126,
                                                    127,
                                                    128,
                                                    129,
                                                    130,
                                                    131,
                                                    132,
                                                    133,
                                                    134,
                                                    135,
                                                    136,
                                                    137,
                                                    138,
                                                    139,
                                                    140,
                                                    141,
                                                    142,
                                                    143,
                                                    144,
                                                    145,
                                                    146,
                                                    147,
                                                    148,
                                                    149,
                                                    150,
                                                    151,
                                                    152,
                                                    153,
                                                    154,
                                                    155,
                                                    156,
                                                    157,
                                                    158,
                                                    159,
                                                    160,
                                                    161,
                                                    162,
                                                    163,
                                                    164,
                                                    165,
                                                    166,
                                                    167,
                                                    168,
                                                    169,
                                                    170,
                                                    171,
                                                    172,
                                                    173,
                                                    174,
                                                    175,
                                                    176,
                                                    177,
                                                    178,
                                                    179,
                                                    180,
                                                    181,
                                                    182,
                                                    183,
                                                    184,
                                                    185,
                                                    186,
                                                    187,
                                                    188,
                                                    189,
                                                    190,
                                                    191,
                                                    224,
                                                    225,
                                                    226,
                                                    227,
                                                    228,
                                                    229,
                                                    230,
                                                    231,  // 192-222 -->
                                                          // 224-254
                                                    232,
                                                    233,
                                                    234,
                                                    235,
                                                    236,
                                                    237,
                                                    238,
                                                    239,
                                                    240,
                                                    241,  // except 215 (times)
                                                    242,
                                                    243,
                                                    244,
                                                    245,
                                                    246,
                                                    215,
                                                    248,
                                                    249,
                                                    250,
                                                    251,
                                                    252,
                                                    253,
                                                    254,
                                                    223,
                                                    224,
                                                    225,
                                                    226,
                                                    227,
                                                    228,
                                                    229,
                                                    230,
                                                    231,
                                                    232,
                                                    233,
                                                    234,
                                                    235,
                                                    236,
                                                    237,
                                                    238,
                                                    239,
                                                    240,
                                                    241,
                                                    242,
                                                    243,
                                                    244,
                                                    245,
                                                    246,
                                                    247,
                                                    248,
                                                    249,
                                                    250,
                                                    251,
                                                    252,
                                                    253,
                                                    254,
                                                    255};

// ----------------------------------------------------------------------
/*!
 * \brief ISO-8859-1 upper case conversion table
 */
// ----------------------------------------------------------------------

static const unsigned char iso_8859_1_upper[256] = {0,
                                                    1,
                                                    2,
                                                    3,
                                                    4,
                                                    5,
                                                    6,
                                                    7,
                                                    8,
                                                    9,
                                                    10,
                                                    11,
                                                    12,
                                                    13,
                                                    14,
                                                    15,
                                                    16,
                                                    17,
                                                    18,
                                                    19,
                                                    20,
                                                    21,
                                                    22,
                                                    23,
                                                    24,
                                                    25,
                                                    26,
                                                    27,
                                                    28,
                                                    29,
                                                    30,
                                                    31,
                                                    32,
                                                    33,
                                                    34,
                                                    35,
                                                    36,
                                                    37,
                                                    38,
                                                    39,
                                                    40,
                                                    41,
                                                    42,
                                                    43,
                                                    44,
                                                    45,
                                                    46,
                                                    47,
                                                    48,
                                                    49,
                                                    50,
                                                    51,
                                                    52,
                                                    53,
                                                    54,
                                                    55,
                                                    56,
                                                    57,
                                                    58,
                                                    59,
                                                    60,
                                                    61,
                                                    62,
                                                    63,
                                                    64,
                                                    65,
                                                    66,
                                                    67,
                                                    68,
                                                    69,
                                                    70,
                                                    71,
                                                    72,
                                                    73,
                                                    74,
                                                    75,
                                                    76,
                                                    77,
                                                    78,
                                                    79,
                                                    80,
                                                    81,
                                                    82,
                                                    83,
                                                    84,
                                                    85,
                                                    86,
                                                    87,
                                                    88,
                                                    89,
                                                    90,
                                                    91,
                                                    92,
                                                    93,
                                                    94,
                                                    95,
                                                    96,
                                                    65,
                                                    66,
                                                    67,  // 97-122 = a-z --> 65-90
                                                    68,
                                                    69,
                                                    70,
                                                    71,
                                                    72,
                                                    73,
                                                    74,
                                                    75,
                                                    76,
                                                    77,
                                                    78,
                                                    79,
                                                    80,
                                                    81,
                                                    82,
                                                    83,
                                                    84,
                                                    85,
                                                    86,
                                                    87,
                                                    88,
                                                    89,
                                                    90,
                                                    123,
                                                    124,
                                                    125,
                                                    126,
                                                    127,
                                                    128,
                                                    129,
                                                    130,
                                                    131,
                                                    132,
                                                    133,
                                                    134,
                                                    135,
                                                    136,
                                                    137,
                                                    138,
                                                    139,
                                                    140,
                                                    141,
                                                    142,
                                                    143,
                                                    144,
                                                    145,
                                                    146,
                                                    147,
                                                    148,
                                                    149,
                                                    150,
                                                    151,
                                                    152,
                                                    153,
                                                    154,
                                                    155,
                                                    156,
                                                    157,
                                                    158,
                                                    159,
                                                    160,
                                                    161,
                                                    162,
                                                    163,
                                                    164,
                                                    165,
                                                    166,
                                                    167,
                                                    168,
                                                    169,
                                                    170,
                                                    171,
                                                    172,
                                                    173,
                                                    174,
                                                    175,
                                                    176,
                                                    177,
                                                    178,
                                                    179,
                                                    180,
                                                    181,
                                                    182,
                                                    183,
                                                    184,
                                                    185,
                                                    186,
                                                    187,
                                                    188,
                                                    189,
                                                    190,
                                                    191,
                                                    192,
                                                    193,
                                                    194,
                                                    195,
                                                    196,
                                                    197,
                                                    198,
                                                    199,
                                                    200,
                                                    201,
                                                    202,
                                                    203,
                                                    204,
                                                    205,
                                                    206,
                                                    207,
                                                    208,
                                                    209,
                                                    210,
                                                    211,
                                                    212,
                                                    213,
                                                    214,
                                                    215,
                                                    216,
                                                    217,
                                                    218,
                                                    219,
                                                    220,
                                                    221,
                                                    222,
                                                    223,
                                                    192,
                                                    193,
                                                    194,
                                                    195,
                                                    196,
                                                    197,  // 224-254 -->
                                                          // 192-222
                                                    198,
                                                    199,
                                                    200,
                                                    201,
                                                    202,
                                                    203,
                                                    204,
                                                    205,
                                                    206,
                                                    207,  // except 247 (divide)
                                                    208,
                                                    209,
                                                    210,
                                                    211,
                                                    212,
                                                    213,
                                                    214,
                                                    247,
                                                    216,
                                                    217,
                                                    218,
                                                    219,
                                                    220,
                                                    221,
                                                    222,
                                                    255};

// ----------------------------------------------------------------------
/*!
 * \brief ISO-8859-1 accent removal conversion table
 *
 * Note: äåö and ÄÅÖ remain unchanged.
 *
 * Accent removals / conversions:
 *
 *  192-195 --> A (65)
 *  198     --> Ä (196)
 *  199     --> C (67)
 *  200-203 --> E (69)
 *  204-207 --> I (73)
 *  209     --> N (78)
 *  210-213 --> O (79)
 *  216     --> Ö (214)
 *  217-220 --> U (85)
 *  221     --> Y (89)
 *  224-227 --> a (97)
 *  230     --> ä (228)
 *  231     --> c (99)
 *  232-235 --> e (101)
 *  236     --> i (105)
 *  241     --> n (110)
 *  242-245 --> o (111)
 *  248     --> ö (246)
 *  249-252 --> u (117)
 *  253     --> y (121)
 *  255     --> y (121)
 */
// ----------------------------------------------------------------------

static const unsigned char iso_8859_1_nordic[256] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,
    19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
    38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,
    57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,
    76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,
    95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
    114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
    133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151,
    152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
    171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
    190, 191, 65,  65,  65,  65,  196, 197, 196, 67,  69,  69,  69,  69,  73,  73,  73,  73,  208,
    78,  79,  79,  79,  79,  214, 215, 214, 85,  85,  85,  85,  89,  222, 223, 97,  97,  97,  97,
    228, 229, 228, 99,  101, 101, 101, 101, 105, 237, 238, 239, 240, 110, 111, 111, 111, 111, 246,
    247, 246, 117, 117, 117, 117, 121, 254, 121};

// ----------------------------------------------------------------------
/*!
 * \brief  ISO-8859-1 MultiByte Char / UTF-8 Conversion Routines
 */
// ----------------------------------------------------------------------

// Map from the most-significant 6 bits of the first byte to the total number of bytes in a UTF-8
// character.
static unsigned char UTF8_2_ISO_8859_1_len[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* erroneous */
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6};

static unsigned char UTF8_2_ISO_8859_1_mask[] = {0x3F, 0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

// ----------------------------------------------------------------------
/*!
 * \brief Platform independent toupper function in Finnish
 */
// ----------------------------------------------------------------------

unsigned char toupper(unsigned char theChar)
{
  return iso_8859_1_upper[static_cast<unsigned char>(theChar)];
}

// ----------------------------------------------------------------------
/*!
 * \brief Platform independent tolower function in Finnish
 */
// ----------------------------------------------------------------------

unsigned char tolower(unsigned char theChar)
{
  return iso_8859_1_lower[static_cast<unsigned char>(theChar)];
}

// ----------------------------------------------------------------------
/*!
 * \brief Platform independent tonordic function in Finnish
 */
// ----------------------------------------------------------------------

unsigned char tonordic(unsigned char theChar)
{
  return iso_8859_1_nordic[static_cast<unsigned char>(theChar)];
}

// ----------------------------------------------------------------------
/*!
 * \brief Platform independent tolowernordic function in Finnish
 */
// ----------------------------------------------------------------------

unsigned char tolowernordic(unsigned char theChar)
{
  return iso_8859_1_nordic[iso_8859_1_lower[static_cast<unsigned char>(theChar)]];
}

// ----------------------------------------------------------------------
/*!
 * \brief Platform independent UTF-8 to ISO-8859-1 decoder
 *
 * returns empty string on failure
 */
// ----------------------------------------------------------------------

string utf8_to_latin1(const string& in)
{
  string out;

  const char* utf8str = in.c_str();

  while (*utf8str != '\0')
  {
    unsigned char len = UTF8_2_ISO_8859_1_len[(*utf8str >> 2) & 0x3F];
    unsigned long u = *utf8str & UTF8_2_ISO_8859_1_mask[len];

    // erroneous -- expect this to be the largest possible UTF-8 encoded string
    if (len == 0) len = 5;

    for (++utf8str; --len > 0 && (*utf8str != '\0'); ++utf8str)
    {
      // be sure this is not an unexpected start of a new character
      if ((*utf8str & 0xc0) != 0x80) break;

      u = (u << 6) | (*utf8str & 0x3f);
    }

    // iso-8859-1 sanity check.
    //
    //  the character has to be between 32 and 255 decimal,
    //  otherwise the utf-8 string will be considered malformed and
    //  the function returns immediately with exit code 0

    if (u > 0xff || u < 0x20) return "";

    // sanity check passed -- add the mapped character to the destination string
    out += static_cast<char>(u);
  }

  return out;
}

// ----------------------------------------------------------------------
/*!
 * \brief Platform independent ISO-8859-1 to UTF-8 encoder
 *
 * Returns the number of bytes in UTF-8 encoded string excl. null-terminator
 */
// ----------------------------------------------------------------------

string latin1_to_utf8(const string& in)
{
  string out;

  const char* mbstr = in.c_str();

  // loop until we reach the end of the mb string
  for (; *mbstr != '\0'; ++mbstr)
  {
    // the character needs no mapping if the highest bit is not set
    if ((*mbstr & 0x80) == 0)
    {
      out += *mbstr;
    }
    // otherwise mapping is necessary (characters 0x80 or greater; highest bit is set)
    else
    {
      out += static_cast<char>((0xC0 | (0x03 & (*mbstr >> 6))));
      out += static_cast<char>((0x80 | (0x3F & *mbstr)));
    }
  }
  return out;
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert UTF-8 string to UTF-16 wstring
 *
 * Ref:
 * http://www.linuxquestions.org/questions/programming-9/wstring-utf8-conversion-in-pure-c-701084/
 */
// ----------------------------------------------------------------------

std::wstring utf8_to_utf16(const std::string& str)
{
  std::wstring out;

  wchar_t w = 0;
  int bytes = 0;
  wchar_t err = L'?';

  for (std::string::size_type i = 0; i < str.size(); i++)
  {
    unsigned char c = static_cast<unsigned char>(str[i]);

    if (c <= 0x7f)
    {
      // first byte
      if (bytes)
      {
        out.push_back(err);
        bytes = 0;
      }
      out.push_back(static_cast<wchar_t>(c));
    }
    else if (c <= 0xbf)
    {
      // second/third/etc byte
      if (bytes)
      {
        w = ((w << 6) | (c & 0x3f));
        bytes--;
        if (bytes == 0) out.push_back(w);
      }
      else
        out.push_back(err);
    }
    else if (c <= 0xdf)
    {
      // 2byte sequence start
      bytes = 1;
      w = c & 0x1f;
    }
    else if (c <= 0xef)
    {
      // 3byte sequence start
      bytes = 2;
      w = c & 0x0f;
    }
    else if (c <= 0xf7)
    {
      // 3byte sequence start
      bytes = 3;
      w = c & 0x07;
    }
    else
    {
      out.push_back(err);
      bytes = 0;
    }
  }
  if (bytes) out.push_back(err);

  return out;
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert UTF-16 wstring to UTF-8 string
 *
 * Ref:
 * http://www.linuxquestions.org/questions/programming-9/wstring-utf8-conversion-in-pure-c-701084/
 */
// ----------------------------------------------------------------------

std::string utf16_to_utf8(const std::wstring& str)
{
  std::string out;

  for (std::wstring::size_type i = 0; i < str.size(); i++)
  {
    wchar_t w = str[i];
    if (w <= 0x7f)
      out.push_back(static_cast<char>(w));
    else if (w <= 0x7ff)
    {
      out.push_back(static_cast<char>(0xc0 | ((w >> 6) & 0x1f)));
      out.push_back(static_cast<char>(0x80 | (w & 0x3f)));
    }
    else if (w <= 0xffff)
    {
      out.push_back(static_cast<char>(0xe0 | ((w >> 12) & 0x0f)));
      out.push_back(static_cast<char>(0x80 | ((w >> 6) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | (w & 0x3f)));
    }
    else if (w <= 0x10ffff)
    {
      out.push_back(static_cast<char>(0xf0 | ((w >> 18) & 0x07)));
      out.push_back(static_cast<char>(0x80 | ((w >> 12) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | ((w >> 6) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | (w & 0x3f)));
    }
    else
      out.push_back('?');
  }

  return out;
}

}  // namespace Fmi

// ======================================================================
