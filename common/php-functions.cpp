// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/php-functions.h"

const uint8_t impl::php_ascii_char_props[256] = {
  0, // '\x00'
  0, // '\x01'
  0, // '\x02'
  0, // '\x03'
  0, // '\x04'
  0, // '\x05'
  0, // '\x06'
  0, // '\a'
  0, // '\b'
  CHAR_PROP_SPACE, // '\t'
  CHAR_PROP_SPACE, // '\n'
  CHAR_PROP_SPACE, // '\v'
  CHAR_PROP_SPACE, // '\f'
  CHAR_PROP_SPACE, // '\r'
  0, // '\x0e'
  0, // '\x0f'
  0, // '\x10'
  0, // '\x11'
  0, // '\x12'
  0, // '\x13'
  0, // '\x14'
  0, // '\x15'
  0, // '\x16'
  0, // '\x17'
  0, // '\x18'
  0, // '\x19'
  0, // '\x1a'
  0, // '\x1b'
  0, // '\x1c'
  0, // '\x1d'
  0, // '\x1e'
  0, // '\x1f'
  CHAR_PROP_SPACE, // ' '
  CHAR_PROP_PUNCT, // '!'
  CHAR_PROP_PUNCT, // '"'
  CHAR_PROP_PUNCT, // '#'
  CHAR_PROP_PUNCT, // '$'
  CHAR_PROP_PUNCT, // '%'
  CHAR_PROP_PUNCT, // '&'
  CHAR_PROP_PUNCT, // '\''
  CHAR_PROP_PUNCT, // '('
  CHAR_PROP_PUNCT, // ')'
  CHAR_PROP_PUNCT, // '*'
  CHAR_PROP_PUNCT, // '+'
  CHAR_PROP_PUNCT, // ','
  CHAR_PROP_PUNCT, // '-'
  CHAR_PROP_PUNCT, // '.'
  CHAR_PROP_PUNCT, // '/'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '0'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '1'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '2'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '3'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '4'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '5'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '6'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '7'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '8'
  CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // '9'
  CHAR_PROP_PUNCT, // ':'
  CHAR_PROP_PUNCT, // ';'
  CHAR_PROP_PUNCT, // '<'
  CHAR_PROP_PUNCT, // '='
  CHAR_PROP_PUNCT, // '>'
  CHAR_PROP_PUNCT, // '?'
  CHAR_PROP_PUNCT, // '@'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'A'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'B'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'C'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'D'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'E'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'F'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'G'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'H'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'I'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'J'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'K'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'L'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'M'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'N'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'O'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'P'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'Q'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'R'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'S'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'T'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'U'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'V'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'W'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'X'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'Y'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'Z'
  CHAR_PROP_PUNCT, // '['
  CHAR_PROP_PUNCT, // '\\'
  CHAR_PROP_PUNCT, // ']'
  CHAR_PROP_PUNCT, // '^'
  CHAR_PROP_PUNCT, // '_'
  CHAR_PROP_PUNCT, // '`'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'a'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'b'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'c'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'd'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'e'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM | CHAR_PROP_XDIGIT, // 'f'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'g'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'h'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'i'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'j'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'k'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'l'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'm'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'n'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'o'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'p'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'q'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'r'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 's'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 't'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'u'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'v'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'w'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'x'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'y'
  CHAR_PROP_ALPHA | CHAR_PROP_ALNUM, // 'z'
  CHAR_PROP_PUNCT, // '{'
  CHAR_PROP_PUNCT, // '|'
  CHAR_PROP_PUNCT, // '}'
  CHAR_PROP_PUNCT, // '~'
  0, // '\x7f'
  0, // '\x80'
  0, // '\x81'
  0, // '\x82'
  0, // '\x83'
  0, // '\x84'
  0, // '\x85'
  0, // '\x86'
  0, // '\x87'
  0, // '\x88'
  0, // '\x89'
  0, // '\x8a'
  0, // '\x8b'
  0, // '\x8c'
  0, // '\x8d'
  0, // '\x8e'
  0, // '\x8f'
  0, // '\x90'
  0, // '\x91'
  0, // '\x92'
  0, // '\x93'
  0, // '\x94'
  0, // '\x95'
  0, // '\x96'
  0, // '\x97'
  0, // '\x98'
  0, // '\x99'
  0, // '\x9a'
  0, // '\x9b'
  0, // '\x9c'
  0, // '\x9d'
  0, // '\x9e'
  0, // '\x9f'
  0, // '\xa0'
  0, // '\xa1'
  0, // '\xa2'
  0, // '\xa3'
  0, // '\xa4'
  0, // '\xa5'
  0, // '\xa6'
  0, // '\xa7'
  0, // '\xa8'
  0, // '\xa9'
  0, // '\xaa'
  0, // '\xab'
  0, // '\xac'
  0, // '\xad'
  0, // '\xae'
  0, // '\xaf'
  0, // '\xb0'
  0, // '\xb1'
  0, // '\xb2'
  0, // '\xb3'
  0, // '\xb4'
  0, // '\xb5'
  0, // '\xb6'
  0, // '\xb7'
  0, // '\xb8'
  0, // '\xb9'
  0, // '\xba'
  0, // '\xbb'
  0, // '\xbc'
  0, // '\xbd'
  0, // '\xbe'
  0, // '\xbf'
  0, // '\xc0'
  0, // '\xc1'
  0, // '\xc2'
  0, // '\xc3'
  0, // '\xc4'
  0, // '\xc5'
  0, // '\xc6'
  0, // '\xc7'
  0, // '\xc8'
  0, // '\xc9'
  0, // '\xca'
  0, // '\xcb'
  0, // '\xcc'
  0, // '\xcd'
  0, // '\xce'
  0, // '\xcf'
  0, // '\xd0'
  0, // '\xd1'
  0, // '\xd2'
  0, // '\xd3'
  0, // '\xd4'
  0, // '\xd5'
  0, // '\xd6'
  0, // '\xd7'
  0, // '\xd8'
  0, // '\xd9'
  0, // '\xda'
  0, // '\xdb'
  0, // '\xdc'
  0, // '\xdd'
  0, // '\xde'
  0, // '\xdf'
  0, // '\xe0'
  0, // '\xe1'
  0, // '\xe2'
  0, // '\xe3'
  0, // '\xe4'
  0, // '\xe5'
  0, // '\xe6'
  0, // '\xe7'
  0, // '\xe8'
  0, // '\xe9'
  0, // '\xea'
  0, // '\xeb'
  0, // '\xec'
  0, // '\xed'
  0, // '\xee'
  0, // '\xef'
  0, // '\xf0'
  0, // '\xf1'
  0, // '\xf2'
  0, // '\xf3'
  0, // '\xf4'
  0, // '\xf5'
  0, // '\xf6'
  0, // '\xf7'
  0, // '\xf8'
  0, // '\xf9'
  0, // '\xfa'
  0, // '\xfb'
  0, // '\xfc'
  0, // '\xfd'
  0, // '\xfe'
  0, // '\xff'
};

const unsigned char impl::php_tolower_conv_table[256] = {
    0, // '\x00' | [  0] 0x00 unchanged
    1, // '\x01' | [  1] 0x01 unchanged
    2, // '\x02' | [  2] 0x02 unchanged
    3, // '\x03' | [  3] 0x03 unchanged
    4, // '\x04' | [  4] 0x04 unchanged
    5, // '\x05' | [  5] 0x05 unchanged
    6, // '\x06' | [  6] 0x06 unchanged
    7, // '\a'   | [  7] 0x07 unchanged
    8, // '\b'   | [  8] 0x08 unchanged
    9, // '\t'   | [  9] 0x09 unchanged
   10, // '\n'   | [ 10] 0x0A unchanged
   11, // '\v'   | [ 11] 0x0B unchanged
   12, // '\f'   | [ 12] 0x0C unchanged
   13, // '\r'   | [ 13] 0x0D unchanged
   14, // '\x0e' | [ 14] 0x0E unchanged
   15, // '\x0f' | [ 15] 0x0F unchanged
   16, // '\x10' | [ 16] 0x10 unchanged
   17, // '\x11' | [ 17] 0x11 unchanged
   18, // '\x12' | [ 18] 0x12 unchanged
   19, // '\x13' | [ 19] 0x13 unchanged
   20, // '\x14' | [ 20] 0x14 unchanged
   21, // '\x15' | [ 21] 0x15 unchanged
   22, // '\x16' | [ 22] 0x16 unchanged
   23, // '\x17' | [ 23] 0x17 unchanged
   24, // '\x18' | [ 24] 0x18 unchanged
   25, // '\x19' | [ 25] 0x19 unchanged
   26, // '\x1a' | [ 26] 0x1A unchanged
   27, // '\x1b' | [ 27] 0x1B unchanged
   28, // '\x1c' | [ 28] 0x1C unchanged
   29, // '\x1d' | [ 29] 0x1D unchanged
   30, // '\x1e' | [ 30] 0x1E unchanged
   31, // '\x1f' | [ 31] 0x1F unchanged
   32, // ' '    | [ 32] 0x20 unchanged
   33, // '!'    | [ 33] 0x21 unchanged
   34, // '"'    | [ 34] 0x22 unchanged
   35, // '#'    | [ 35] 0x23 unchanged
   36, // '$'    | [ 36] 0x24 unchanged
   37, // '%'    | [ 37] 0x25 unchanged
   38, // '&'    | [ 38] 0x26 unchanged
   39, // '\''   | [ 39] 0x27 unchanged
   40, // '('    | [ 40] 0x28 unchanged
   41, // ')'    | [ 41] 0x29 unchanged
   42, // '*'    | [ 42] 0x2A unchanged
   43, // '+'    | [ 43] 0x2B unchanged
   44, // ','    | [ 44] 0x2C unchanged
   45, // '-'    | [ 45] 0x2D unchanged
   46, // '.'    | [ 46] 0x2E unchanged
   47, // '/'    | [ 47] 0x2F unchanged
   48, // '0'    | [ 48] 0x30 unchanged
   49, // '1'    | [ 49] 0x31 unchanged
   50, // '2'    | [ 50] 0x32 unchanged
   51, // '3'    | [ 51] 0x33 unchanged
   52, // '4'    | [ 52] 0x34 unchanged
   53, // '5'    | [ 53] 0x35 unchanged
   54, // '6'    | [ 54] 0x36 unchanged
   55, // '7'    | [ 55] 0x37 unchanged
   56, // '8'    | [ 56] 0x38 unchanged
   57, // '9'    | [ 57] 0x39 unchanged
   58, // ':'    | [ 58] 0x3A unchanged
   59, // ';'    | [ 59] 0x3B unchanged
   60, // '<'    | [ 60] 0x3C unchanged
   61, // '='    | [ 61] 0x3D unchanged
   62, // '>'    | [ 62] 0x3E unchanged
   63, // '?'    | [ 63] 0x3F unchanged
   64, // '@'    | [ 64] 0x40 unchanged
   97, // 'A'    | [ 65] 0x41 => 0x61 [ 97]
   98, // 'B'    | [ 66] 0x42 => 0x62 [ 98]
   99, // 'C'    | [ 67] 0x43 => 0x63 [ 99]
  100, // 'D'    | [ 68] 0x44 => 0x64 [100]
  101, // 'E'    | [ 69] 0x45 => 0x65 [101]
  102, // 'F'    | [ 70] 0x46 => 0x66 [102]
  103, // 'G'    | [ 71] 0x47 => 0x67 [103]
  104, // 'H'    | [ 72] 0x48 => 0x68 [104]
  105, // 'I'    | [ 73] 0x49 => 0x69 [105]
  106, // 'J'    | [ 74] 0x4A => 0x6A [106]
  107, // 'K'    | [ 75] 0x4B => 0x6B [107]
  108, // 'L'    | [ 76] 0x4C => 0x6C [108]
  109, // 'M'    | [ 77] 0x4D => 0x6D [109]
  110, // 'N'    | [ 78] 0x4E => 0x6E [110]
  111, // 'O'    | [ 79] 0x4F => 0x6F [111]
  112, // 'P'    | [ 80] 0x50 => 0x70 [112]
  113, // 'Q'    | [ 81] 0x51 => 0x71 [113]
  114, // 'R'    | [ 82] 0x52 => 0x72 [114]
  115, // 'S'    | [ 83] 0x53 => 0x73 [115]
  116, // 'T'    | [ 84] 0x54 => 0x74 [116]
  117, // 'U'    | [ 85] 0x55 => 0x75 [117]
  118, // 'V'    | [ 86] 0x56 => 0x76 [118]
  119, // 'W'    | [ 87] 0x57 => 0x77 [119]
  120, // 'X'    | [ 88] 0x58 => 0x78 [120]
  121, // 'Y'    | [ 89] 0x59 => 0x79 [121]
  122, // 'Z'    | [ 90] 0x5A => 0x7A [122]
   91, // '['    | [ 91] 0x5B unchanged
   92, // '\\'   | [ 92] 0x5C unchanged
   93, // ']'    | [ 93] 0x5D unchanged
   94, // '^'    | [ 94] 0x5E unchanged
   95, // '_'    | [ 95] 0x5F unchanged
   96, // '`'    | [ 96] 0x60 unchanged
   97, // 'a'    | [ 97] 0x61 unchanged
   98, // 'b'    | [ 98] 0x62 unchanged
   99, // 'c'    | [ 99] 0x63 unchanged
  100, // 'd'    | [100] 0x64 unchanged
  101, // 'e'    | [101] 0x65 unchanged
  102, // 'f'    | [102] 0x66 unchanged
  103, // 'g'    | [103] 0x67 unchanged
  104, // 'h'    | [104] 0x68 unchanged
  105, // 'i'    | [105] 0x69 unchanged
  106, // 'j'    | [106] 0x6A unchanged
  107, // 'k'    | [107] 0x6B unchanged
  108, // 'l'    | [108] 0x6C unchanged
  109, // 'm'    | [109] 0x6D unchanged
  110, // 'n'    | [110] 0x6E unchanged
  111, // 'o'    | [111] 0x6F unchanged
  112, // 'p'    | [112] 0x70 unchanged
  113, // 'q'    | [113] 0x71 unchanged
  114, // 'r'    | [114] 0x72 unchanged
  115, // 's'    | [115] 0x73 unchanged
  116, // 't'    | [116] 0x74 unchanged
  117, // 'u'    | [117] 0x75 unchanged
  118, // 'v'    | [118] 0x76 unchanged
  119, // 'w'    | [119] 0x77 unchanged
  120, // 'x'    | [120] 0x78 unchanged
  121, // 'y'    | [121] 0x79 unchanged
  122, // 'z'    | [122] 0x7A unchanged
  123, // '{'    | [123] 0x7B unchanged
  124, // '|'    | [124] 0x7C unchanged
  125, // '}'    | [125] 0x7D unchanged
  126, // '~'    | [126] 0x7E unchanged
  127, // '\x7f' | [127] 0x7F unchanged
  128, // '\x80' | [128] 0x80 unchanged
  129, // '\x81' | [129] 0x81 unchanged
  130, // '\x82' | [130] 0x82 unchanged
  131, // '\x83' | [131] 0x83 unchanged
  132, // '\x84' | [132] 0x84 unchanged
  133, // '\x85' | [133] 0x85 unchanged
  134, // '\x86' | [134] 0x86 unchanged
  135, // '\x87' | [135] 0x87 unchanged
  136, // '\x88' | [136] 0x88 unchanged
  137, // '\x89' | [137] 0x89 unchanged
  138, // '\x8a' | [138] 0x8A unchanged
  139, // '\x8b' | [139] 0x8B unchanged
  140, // '\x8c' | [140] 0x8C unchanged
  141, // '\x8d' | [141] 0x8D unchanged
  142, // '\x8e' | [142] 0x8E unchanged
  143, // '\x8f' | [143] 0x8F unchanged
  144, // '\x90' | [144] 0x90 unchanged
  145, // '\x91' | [145] 0x91 unchanged
  146, // '\x92' | [146] 0x92 unchanged
  147, // '\x93' | [147] 0x93 unchanged
  148, // '\x94' | [148] 0x94 unchanged
  149, // '\x95' | [149] 0x95 unchanged
  150, // '\x96' | [150] 0x96 unchanged
  151, // '\x97' | [151] 0x97 unchanged
  152, // '\x98' | [152] 0x98 unchanged
  153, // '\x99' | [153] 0x99 unchanged
  154, // '\x9a' | [154] 0x9A unchanged
  155, // '\x9b' | [155] 0x9B unchanged
  156, // '\x9c' | [156] 0x9C unchanged
  157, // '\x9d' | [157] 0x9D unchanged
  158, // '\x9e' | [158] 0x9E unchanged
  159, // '\x9f' | [159] 0x9F unchanged
  160, // '\xa0' | [160] 0xA0 unchanged
  161, // '\xa1' | [161] 0xA1 unchanged
  162, // '\xa2' | [162] 0xA2 unchanged
  163, // '\xa3' | [163] 0xA3 unchanged
  164, // '\xa4' | [164] 0xA4 unchanged
  165, // '\xa5' | [165] 0xA5 unchanged
  166, // '\xa6' | [166] 0xA6 unchanged
  167, // '\xa7' | [167] 0xA7 unchanged
  168, // '\xa8' | [168] 0xA8 unchanged
  169, // '\xa9' | [169] 0xA9 unchanged
  170, // '\xaa' | [170] 0xAA unchanged
  171, // '\xab' | [171] 0xAB unchanged
  172, // '\xac' | [172] 0xAC unchanged
  173, // '\xad' | [173] 0xAD unchanged
  174, // '\xae' | [174] 0xAE unchanged
  175, // '\xaf' | [175] 0xAF unchanged
  176, // '\xb0' | [176] 0xB0 unchanged
  177, // '\xb1' | [177] 0xB1 unchanged
  178, // '\xb2' | [178] 0xB2 unchanged
  179, // '\xb3' | [179] 0xB3 unchanged
  180, // '\xb4' | [180] 0xB4 unchanged
  181, // '\xb5' | [181] 0xB5 unchanged
  182, // '\xb6' | [182] 0xB6 unchanged
  183, // '\xb7' | [183] 0xB7 unchanged
  184, // '\xb8' | [184] 0xB8 unchanged
  185, // '\xb9' | [185] 0xB9 unchanged
  186, // '\xba' | [186] 0xBA unchanged
  187, // '\xbb' | [187] 0xBB unchanged
  188, // '\xbc' | [188] 0xBC unchanged
  189, // '\xbd' | [189] 0xBD unchanged
  190, // '\xbe' | [190] 0xBE unchanged
  191, // '\xbf' | [191] 0xBF unchanged
  192, // '\xc0' | [192] 0xC0 unchanged
  193, // '\xc1' | [193] 0xC1 unchanged
  194, // '\xc2' | [194] 0xC2 unchanged
  195, // '\xc3' | [195] 0xC3 unchanged
  196, // '\xc4' | [196] 0xC4 unchanged
  197, // '\xc5' | [197] 0xC5 unchanged
  198, // '\xc6' | [198] 0xC6 unchanged
  199, // '\xc7' | [199] 0xC7 unchanged
  200, // '\xc8' | [200] 0xC8 unchanged
  201, // '\xc9' | [201] 0xC9 unchanged
  202, // '\xca' | [202] 0xCA unchanged
  203, // '\xcb' | [203] 0xCB unchanged
  204, // '\xcc' | [204] 0xCC unchanged
  205, // '\xcd' | [205] 0xCD unchanged
  206, // '\xce' | [206] 0xCE unchanged
  207, // '\xcf' | [207] 0xCF unchanged
  208, // '\xd0' | [208] 0xD0 unchanged
  209, // '\xd1' | [209] 0xD1 unchanged
  210, // '\xd2' | [210] 0xD2 unchanged
  211, // '\xd3' | [211] 0xD3 unchanged
  212, // '\xd4' | [212] 0xD4 unchanged
  213, // '\xd5' | [213] 0xD5 unchanged
  214, // '\xd6' | [214] 0xD6 unchanged
  215, // '\xd7' | [215] 0xD7 unchanged
  216, // '\xd8' | [216] 0xD8 unchanged
  217, // '\xd9' | [217] 0xD9 unchanged
  218, // '\xda' | [218] 0xDA unchanged
  219, // '\xdb' | [219] 0xDB unchanged
  220, // '\xdc' | [220] 0xDC unchanged
  221, // '\xdd' | [221] 0xDD unchanged
  222, // '\xde' | [222] 0xDE unchanged
  223, // '\xdf' | [223] 0xDF unchanged
  224, // '\xe0' | [224] 0xE0 unchanged
  225, // '\xe1' | [225] 0xE1 unchanged
  226, // '\xe2' | [226] 0xE2 unchanged
  227, // '\xe3' | [227] 0xE3 unchanged
  228, // '\xe4' | [228] 0xE4 unchanged
  229, // '\xe5' | [229] 0xE5 unchanged
  230, // '\xe6' | [230] 0xE6 unchanged
  231, // '\xe7' | [231] 0xE7 unchanged
  232, // '\xe8' | [232] 0xE8 unchanged
  233, // '\xe9' | [233] 0xE9 unchanged
  234, // '\xea' | [234] 0xEA unchanged
  235, // '\xeb' | [235] 0xEB unchanged
  236, // '\xec' | [236] 0xEC unchanged
  237, // '\xed' | [237] 0xED unchanged
  238, // '\xee' | [238] 0xEE unchanged
  239, // '\xef' | [239] 0xEF unchanged
  240, // '\xf0' | [240] 0xF0 unchanged
  241, // '\xf1' | [241] 0xF1 unchanged
  242, // '\xf2' | [242] 0xF2 unchanged
  243, // '\xf3' | [243] 0xF3 unchanged
  244, // '\xf4' | [244] 0xF4 unchanged
  245, // '\xf5' | [245] 0xF5 unchanged
  246, // '\xf6' | [246] 0xF6 unchanged
  247, // '\xf7' | [247] 0xF7 unchanged
  248, // '\xf8' | [248] 0xF8 unchanged
  249, // '\xf9' | [249] 0xF9 unchanged
  250, // '\xfa' | [250] 0xFA unchanged
  251, // '\xfb' | [251] 0xFB unchanged
  252, // '\xfc' | [252] 0xFC unchanged
  253, // '\xfd' | [253] 0xFD unchanged
  254, // '\xfe' | [254] 0xFE unchanged
  255, // '\xff' | [255] 0xFF unchanged
};

const unsigned char impl::php_toupper_conv_table[256] = {
    0, // '\x00' | [  0] 0x00 unchanged
    1, // '\x01' | [  1] 0x01 unchanged
    2, // '\x02' | [  2] 0x02 unchanged
    3, // '\x03' | [  3] 0x03 unchanged
    4, // '\x04' | [  4] 0x04 unchanged
    5, // '\x05' | [  5] 0x05 unchanged
    6, // '\x06' | [  6] 0x06 unchanged
    7, // '\a'   | [  7] 0x07 unchanged
    8, // '\b'   | [  8] 0x08 unchanged
    9, // '\t'   | [  9] 0x09 unchanged
   10, // '\n'   | [ 10] 0x0A unchanged
   11, // '\v'   | [ 11] 0x0B unchanged
   12, // '\f'   | [ 12] 0x0C unchanged
   13, // '\r'   | [ 13] 0x0D unchanged
   14, // '\x0e' | [ 14] 0x0E unchanged
   15, // '\x0f' | [ 15] 0x0F unchanged
   16, // '\x10' | [ 16] 0x10 unchanged
   17, // '\x11' | [ 17] 0x11 unchanged
   18, // '\x12' | [ 18] 0x12 unchanged
   19, // '\x13' | [ 19] 0x13 unchanged
   20, // '\x14' | [ 20] 0x14 unchanged
   21, // '\x15' | [ 21] 0x15 unchanged
   22, // '\x16' | [ 22] 0x16 unchanged
   23, // '\x17' | [ 23] 0x17 unchanged
   24, // '\x18' | [ 24] 0x18 unchanged
   25, // '\x19' | [ 25] 0x19 unchanged
   26, // '\x1a' | [ 26] 0x1A unchanged
   27, // '\x1b' | [ 27] 0x1B unchanged
   28, // '\x1c' | [ 28] 0x1C unchanged
   29, // '\x1d' | [ 29] 0x1D unchanged
   30, // '\x1e' | [ 30] 0x1E unchanged
   31, // '\x1f' | [ 31] 0x1F unchanged
   32, // ' '    | [ 32] 0x20 unchanged
   33, // '!'    | [ 33] 0x21 unchanged
   34, // '"'    | [ 34] 0x22 unchanged
   35, // '#'    | [ 35] 0x23 unchanged
   36, // '$'    | [ 36] 0x24 unchanged
   37, // '%'    | [ 37] 0x25 unchanged
   38, // '&'    | [ 38] 0x26 unchanged
   39, // '\''   | [ 39] 0x27 unchanged
   40, // '('    | [ 40] 0x28 unchanged
   41, // ')'    | [ 41] 0x29 unchanged
   42, // '*'    | [ 42] 0x2A unchanged
   43, // '+'    | [ 43] 0x2B unchanged
   44, // ','    | [ 44] 0x2C unchanged
   45, // '-'    | [ 45] 0x2D unchanged
   46, // '.'    | [ 46] 0x2E unchanged
   47, // '/'    | [ 47] 0x2F unchanged
   48, // '0'    | [ 48] 0x30 unchanged
   49, // '1'    | [ 49] 0x31 unchanged
   50, // '2'    | [ 50] 0x32 unchanged
   51, // '3'    | [ 51] 0x33 unchanged
   52, // '4'    | [ 52] 0x34 unchanged
   53, // '5'    | [ 53] 0x35 unchanged
   54, // '6'    | [ 54] 0x36 unchanged
   55, // '7'    | [ 55] 0x37 unchanged
   56, // '8'    | [ 56] 0x38 unchanged
   57, // '9'    | [ 57] 0x39 unchanged
   58, // ':'    | [ 58] 0x3A unchanged
   59, // ';'    | [ 59] 0x3B unchanged
   60, // '<'    | [ 60] 0x3C unchanged
   61, // '='    | [ 61] 0x3D unchanged
   62, // '>'    | [ 62] 0x3E unchanged
   63, // '?'    | [ 63] 0x3F unchanged
   64, // '@'    | [ 64] 0x40 unchanged
   65, // 'A'    | [ 65] 0x41 unchanged
   66, // 'B'    | [ 66] 0x42 unchanged
   67, // 'C'    | [ 67] 0x43 unchanged
   68, // 'D'    | [ 68] 0x44 unchanged
   69, // 'E'    | [ 69] 0x45 unchanged
   70, // 'F'    | [ 70] 0x46 unchanged
   71, // 'G'    | [ 71] 0x47 unchanged
   72, // 'H'    | [ 72] 0x48 unchanged
   73, // 'I'    | [ 73] 0x49 unchanged
   74, // 'J'    | [ 74] 0x4A unchanged
   75, // 'K'    | [ 75] 0x4B unchanged
   76, // 'L'    | [ 76] 0x4C unchanged
   77, // 'M'    | [ 77] 0x4D unchanged
   78, // 'N'    | [ 78] 0x4E unchanged
   79, // 'O'    | [ 79] 0x4F unchanged
   80, // 'P'    | [ 80] 0x50 unchanged
   81, // 'Q'    | [ 81] 0x51 unchanged
   82, // 'R'    | [ 82] 0x52 unchanged
   83, // 'S'    | [ 83] 0x53 unchanged
   84, // 'T'    | [ 84] 0x54 unchanged
   85, // 'U'    | [ 85] 0x55 unchanged
   86, // 'V'    | [ 86] 0x56 unchanged
   87, // 'W'    | [ 87] 0x57 unchanged
   88, // 'X'    | [ 88] 0x58 unchanged
   89, // 'Y'    | [ 89] 0x59 unchanged
   90, // 'Z'    | [ 90] 0x5A unchanged
   91, // '['    | [ 91] 0x5B unchanged
   92, // '\\'   | [ 92] 0x5C unchanged
   93, // ']'    | [ 93] 0x5D unchanged
   94, // '^'    | [ 94] 0x5E unchanged
   95, // '_'    | [ 95] 0x5F unchanged
   96, // '`'    | [ 96] 0x60 unchanged
   65, // 'a'    | [ 97] 0x61 => 0x41 [ 65]
   66, // 'b'    | [ 98] 0x62 => 0x42 [ 66]
   67, // 'c'    | [ 99] 0x63 => 0x43 [ 67]
   68, // 'd'    | [100] 0x64 => 0x44 [ 68]
   69, // 'e'    | [101] 0x65 => 0x45 [ 69]
   70, // 'f'    | [102] 0x66 => 0x46 [ 70]
   71, // 'g'    | [103] 0x67 => 0x47 [ 71]
   72, // 'h'    | [104] 0x68 => 0x48 [ 72]
   73, // 'i'    | [105] 0x69 => 0x49 [ 73]
   74, // 'j'    | [106] 0x6A => 0x4A [ 74]
   75, // 'k'    | [107] 0x6B => 0x4B [ 75]
   76, // 'l'    | [108] 0x6C => 0x4C [ 76]
   77, // 'm'    | [109] 0x6D => 0x4D [ 77]
   78, // 'n'    | [110] 0x6E => 0x4E [ 78]
   79, // 'o'    | [111] 0x6F => 0x4F [ 79]
   80, // 'p'    | [112] 0x70 => 0x50 [ 80]
   81, // 'q'    | [113] 0x71 => 0x51 [ 81]
   82, // 'r'    | [114] 0x72 => 0x52 [ 82]
   83, // 's'    | [115] 0x73 => 0x53 [ 83]
   84, // 't'    | [116] 0x74 => 0x54 [ 84]
   85, // 'u'    | [117] 0x75 => 0x55 [ 85]
   86, // 'v'    | [118] 0x76 => 0x56 [ 86]
   87, // 'w'    | [119] 0x77 => 0x57 [ 87]
   88, // 'x'    | [120] 0x78 => 0x58 [ 88]
   89, // 'y'    | [121] 0x79 => 0x59 [ 89]
   90, // 'z'    | [122] 0x7A => 0x5A [ 90]
  123, // '{'    | [123] 0x7B unchanged
  124, // '|'    | [124] 0x7C unchanged
  125, // '}'    | [125] 0x7D unchanged
  126, // '~'    | [126] 0x7E unchanged
  127, // '\x7f' | [127] 0x7F unchanged
  128, // '\x80' | [128] 0x80 unchanged
  129, // '\x81' | [129] 0x81 unchanged
  130, // '\x82' | [130] 0x82 unchanged
  131, // '\x83' | [131] 0x83 unchanged
  132, // '\x84' | [132] 0x84 unchanged
  133, // '\x85' | [133] 0x85 unchanged
  134, // '\x86' | [134] 0x86 unchanged
  135, // '\x87' | [135] 0x87 unchanged
  136, // '\x88' | [136] 0x88 unchanged
  137, // '\x89' | [137] 0x89 unchanged
  138, // '\x8a' | [138] 0x8A unchanged
  139, // '\x8b' | [139] 0x8B unchanged
  140, // '\x8c' | [140] 0x8C unchanged
  141, // '\x8d' | [141] 0x8D unchanged
  142, // '\x8e' | [142] 0x8E unchanged
  143, // '\x8f' | [143] 0x8F unchanged
  144, // '\x90' | [144] 0x90 unchanged
  145, // '\x91' | [145] 0x91 unchanged
  146, // '\x92' | [146] 0x92 unchanged
  147, // '\x93' | [147] 0x93 unchanged
  148, // '\x94' | [148] 0x94 unchanged
  149, // '\x95' | [149] 0x95 unchanged
  150, // '\x96' | [150] 0x96 unchanged
  151, // '\x97' | [151] 0x97 unchanged
  152, // '\x98' | [152] 0x98 unchanged
  153, // '\x99' | [153] 0x99 unchanged
  154, // '\x9a' | [154] 0x9A unchanged
  155, // '\x9b' | [155] 0x9B unchanged
  156, // '\x9c' | [156] 0x9C unchanged
  157, // '\x9d' | [157] 0x9D unchanged
  158, // '\x9e' | [158] 0x9E unchanged
  159, // '\x9f' | [159] 0x9F unchanged
  160, // '\xa0' | [160] 0xA0 unchanged
  161, // '\xa1' | [161] 0xA1 unchanged
  162, // '\xa2' | [162] 0xA2 unchanged
  163, // '\xa3' | [163] 0xA3 unchanged
  164, // '\xa4' | [164] 0xA4 unchanged
  165, // '\xa5' | [165] 0xA5 unchanged
  166, // '\xa6' | [166] 0xA6 unchanged
  167, // '\xa7' | [167] 0xA7 unchanged
  168, // '\xa8' | [168] 0xA8 unchanged
  169, // '\xa9' | [169] 0xA9 unchanged
  170, // '\xaa' | [170] 0xAA unchanged
  171, // '\xab' | [171] 0xAB unchanged
  172, // '\xac' | [172] 0xAC unchanged
  173, // '\xad' | [173] 0xAD unchanged
  174, // '\xae' | [174] 0xAE unchanged
  175, // '\xaf' | [175] 0xAF unchanged
  176, // '\xb0' | [176] 0xB0 unchanged
  177, // '\xb1' | [177] 0xB1 unchanged
  178, // '\xb2' | [178] 0xB2 unchanged
  179, // '\xb3' | [179] 0xB3 unchanged
  180, // '\xb4' | [180] 0xB4 unchanged
  181, // '\xb5' | [181] 0xB5 unchanged
  182, // '\xb6' | [182] 0xB6 unchanged
  183, // '\xb7' | [183] 0xB7 unchanged
  184, // '\xb8' | [184] 0xB8 unchanged
  185, // '\xb9' | [185] 0xB9 unchanged
  186, // '\xba' | [186] 0xBA unchanged
  187, // '\xbb' | [187] 0xBB unchanged
  188, // '\xbc' | [188] 0xBC unchanged
  189, // '\xbd' | [189] 0xBD unchanged
  190, // '\xbe' | [190] 0xBE unchanged
  191, // '\xbf' | [191] 0xBF unchanged
  192, // '\xc0' | [192] 0xC0 unchanged
  193, // '\xc1' | [193] 0xC1 unchanged
  194, // '\xc2' | [194] 0xC2 unchanged
  195, // '\xc3' | [195] 0xC3 unchanged
  196, // '\xc4' | [196] 0xC4 unchanged
  197, // '\xc5' | [197] 0xC5 unchanged
  198, // '\xc6' | [198] 0xC6 unchanged
  199, // '\xc7' | [199] 0xC7 unchanged
  200, // '\xc8' | [200] 0xC8 unchanged
  201, // '\xc9' | [201] 0xC9 unchanged
  202, // '\xca' | [202] 0xCA unchanged
  203, // '\xcb' | [203] 0xCB unchanged
  204, // '\xcc' | [204] 0xCC unchanged
  205, // '\xcd' | [205] 0xCD unchanged
  206, // '\xce' | [206] 0xCE unchanged
  207, // '\xcf' | [207] 0xCF unchanged
  208, // '\xd0' | [208] 0xD0 unchanged
  209, // '\xd1' | [209] 0xD1 unchanged
  210, // '\xd2' | [210] 0xD2 unchanged
  211, // '\xd3' | [211] 0xD3 unchanged
  212, // '\xd4' | [212] 0xD4 unchanged
  213, // '\xd5' | [213] 0xD5 unchanged
  214, // '\xd6' | [214] 0xD6 unchanged
  215, // '\xd7' | [215] 0xD7 unchanged
  216, // '\xd8' | [216] 0xD8 unchanged
  217, // '\xd9' | [217] 0xD9 unchanged
  218, // '\xda' | [218] 0xDA unchanged
  219, // '\xdb' | [219] 0xDB unchanged
  220, // '\xdc' | [220] 0xDC unchanged
  221, // '\xdd' | [221] 0xDD unchanged
  222, // '\xde' | [222] 0xDE unchanged
  223, // '\xdf' | [223] 0xDF unchanged
  224, // '\xe0' | [224] 0xE0 unchanged
  225, // '\xe1' | [225] 0xE1 unchanged
  226, // '\xe2' | [226] 0xE2 unchanged
  227, // '\xe3' | [227] 0xE3 unchanged
  228, // '\xe4' | [228] 0xE4 unchanged
  229, // '\xe5' | [229] 0xE5 unchanged
  230, // '\xe6' | [230] 0xE6 unchanged
  231, // '\xe7' | [231] 0xE7 unchanged
  232, // '\xe8' | [232] 0xE8 unchanged
  233, // '\xe9' | [233] 0xE9 unchanged
  234, // '\xea' | [234] 0xEA unchanged
  235, // '\xeb' | [235] 0xEB unchanged
  236, // '\xec' | [236] 0xEC unchanged
  237, // '\xed' | [237] 0xED unchanged
  238, // '\xee' | [238] 0xEE unchanged
  239, // '\xef' | [239] 0xEF unchanged
  240, // '\xf0' | [240] 0xF0 unchanged
  241, // '\xf1' | [241] 0xF1 unchanged
  242, // '\xf2' | [242] 0xF2 unchanged
  243, // '\xf3' | [243] 0xF3 unchanged
  244, // '\xf4' | [244] 0xF4 unchanged
  245, // '\xf5' | [245] 0xF5 unchanged
  246, // '\xf6' | [246] 0xF6 unchanged
  247, // '\xf7' | [247] 0xF7 unchanged
  248, // '\xf8' | [248] 0xF8 unchanged
  249, // '\xf9' | [249] 0xF9 unchanged
  250, // '\xfa' | [250] 0xFA unchanged
  251, // '\xfb' | [251] 0xFB unchanged
  252, // '\xfc' | [252] 0xFC unchanged
  253, // '\xfd' | [253] 0xFD unchanged
  254, // '\xfe' | [254] 0xFE unchanged
  255, // '\xff' | [255] 0xFF unchanged
};
