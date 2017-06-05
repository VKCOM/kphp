/* C++ code produced by gperf version 3.0.4 */
/* Command-line: gperf -CGD -c -N get_type -Z KeywordsSet -K keyword -L C++ -t keywords.gperf  */
/* Computed positions: -k'1,3-4' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "keywords.gperf"

#include "token.h"
typedef struct KeywordType_t KeywordType;
#line 5 "keywords.gperf"
struct KeywordType_t
{
  const char *keyword;
  TokenType type;
};

#define TOTAL_KEYWORDS 83
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 15
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 141
/* maximum key range = 140, duplicates = 0 */

class KeywordsSet
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct KeywordType_t *get_type (const char *str, unsigned int len);
};

inline unsigned int
KeywordsSet::hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142,  75, 142,  40,  20,  40,
        0, 142, 142,   5, 142, 142,  30,  25,  35, 142,
      142, 142, 142,   0,  40,   5, 142, 142, 142, 142,
      142, 142,  15, 142, 142,  30, 142,  10,  70,  35,
        0,   0,   5,  25,  20,  15,   5,   0,  30,   5,
       40,  35,  35,   5,  20,   0,   0,  65,   0,  50,
       30,   5, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
      142, 142, 142, 142, 142, 142
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static const struct KeywordType_t wordlist[] =
  {
#line 31 "keywords.gperf"
    {"do", tok_do},
#line 34 "keywords.gperf"
    {"die", tok_exit},
#line 13 "keywords.gperf"
    {"else", tok_else},
#line 14 "keywords.gperf"
    {"elseif", tok_elseif},
#line 93 "keywords.gperf"
    {"extends", tok_extends},
#line 84 "keywords.gperf"
    {"try", tok_try},
#line 25 "keywords.gperf"
    {"as", tok_as},
#line 74 "keywords.gperf"
    {"and", tok_log_and_let},
#line 22 "keywords.gperf"
    {"extern_function", tok_ex_function},
#line 36 "keywords.gperf"
    {"static", tok_static},
#line 12 "keywords.gperf"
    {"if", tok_if},
#line 62 "keywords.gperf"
    {"int", tok_int},
#line 33 "keywords.gperf"
    {"exit", tok_exit},
#line 45 "keywords.gperf"
    {"isset", tok_isset},
#line 27 "keywords.gperf"
    {"switch", tok_switch},
#line 30 "keywords.gperf"
    {"default", tok_default},
#line 55 "keywords.gperf"
    {"var", tok_var},
#line 76 "keywords.gperf"
    {"define", tok_define},
#line 78 "keywords.gperf"
    {"defined", tok_defined},
#line 15 "keywords.gperf"
    {"for", tok_for},
#line 79 "keywords.gperf"
    {"define_raw", tok_define_raw},
#line 77 "keywords.gperf"
    {"DEFINE", tok_define},
#line 16 "keywords.gperf"
    {"foreach", tok_foreach},
#line 39 "keywords.gperf"
    {"list", tok_list},
#line 69 "keywords.gperf"
    {"FALSE", tok_false},
#line 75 "keywords.gperf"
    {"or", tok_log_or_let},
#line 49 "keywords.gperf"
    {"max", tok_max},
#line 26 "keywords.gperf"
    {"case", tok_case},
#line 67 "keywords.gperf"
    {"false", tok_false},
#line 64 "keywords.gperf"
    {"string", tok_string},
#line 51 "keywords.gperf"
    {"sprintf", tok_sprintf},
#line 57 "keywords.gperf"
    {"__FILE__", tok_file_c},
#line 32 "keywords.gperf"
    {"eval", tok_eval},
#line 23 "keywords.gperf"
    {"array", tok_array},
#line 65 "keywords.gperf"
    {"object", tok_object},
#line 58 "keywords.gperf"
    {"__FUNCTION__", tok_func_c},
#line 48 "keywords.gperf"
    {"min", tok_min},
#line 40 "keywords.gperf"
    {"auto", tok_auto},
#line 28 "keywords.gperf"
    {"class", tok_class},
#line 73 "keywords.gperf"
    {"xor", tok_log_xor_let},
#line 88 "keywords.gperf"
    {"namespace", tok_namespace},
#line 63 "keywords.gperf"
    {"float", tok_float},
#line 90 "keywords.gperf"
    {"private", tok_private},
#line 47 "keywords.gperf"
    {"var_dump", tok_var_dump},
#line 19 "keywords.gperf"
    {"echo", tok_echo},
#line 81 "keywords.gperf"
    {"throw", tok_throw},
#line 86 "keywords.gperf"
    {"throws", tok_throws},
#line 59 "keywords.gperf"
    {"__DIR__", tok_dir_c},
#line 20 "keywords.gperf"
    {"dbg_echo", tok_dbg_echo},
#line 37 "keywords.gperf"
    {"goto", tok_goto},
#line 53 "keywords.gperf"
    {"store_many", tok_store_many},
#line 92 "keywords.gperf"
    {"use", tok_use},
#line 68 "keywords.gperf"
    {"true", tok_true},
#line 54 "keywords.gperf"
    {"unset", tok_unset},
#line 60 "keywords.gperf"
    {"__LINE__", tok_line_c},
#line 50 "keywords.gperf"
    {"pack", tok_pack},
#line 85 "keywords.gperf"
    {"catch", tok_catch},
#line 91 "keywords.gperf"
    {"protected", tok_protected},
#line 29 "keywords.gperf"
    {"const", tok_const},
#line 18 "keywords.gperf"
    {"continue", tok_continue},
#line 82 "keywords.gperf"
    {"Exception", tok_Exception},
#line 17 "keywords.gperf"
    {"break", tok_break},
#line 41 "keywords.gperf"
    {"include", tok_require},
#line 21 "keywords.gperf"
    {"function", tok_function},
#line 70 "keywords.gperf"
    {"TRUE", tok_true},
#line 83 "keywords.gperf"
    {"\\Exception", tok_Exception},
#line 38 "keywords.gperf"
    {"return", tok_return},
#line 42 "keywords.gperf"
    {"include_once", tok_require_once},
#line 80 "keywords.gperf"
    {"new", tok_new},
#line 87 "keywords.gperf"
    {"resumable", tok_resumable},
#line 46 "keywords.gperf"
    {"print", tok_print},
#line 52 "keywords.gperf"
    {"printf", tok_printf},
#line 43 "keywords.gperf"
    {"require", tok_require},
#line 71 "keywords.gperf"
    {"NULL", tok_null},
#line 11 "keywords.gperf"
    {"while", tok_while},
#line 44 "keywords.gperf"
    {"require_once", tok_require_once},
#line 72 "keywords.gperf"
    {"null", tok_null},
#line 61 "keywords.gperf"
    {"__METHOD__", tok_method_c},
#line 56 "keywords.gperf"
    {"__CLASS__", tok_class_c},
#line 24 "keywords.gperf"
    {"Array", tok_array},
#line 35 "keywords.gperf"
    {"global", tok_global},
#line 66 "keywords.gperf"
    {"bool", tok_bool},
#line 89 "keywords.gperf"
    {"public", tok_public}
  };

static const signed char lookup[] =
  {
    -1, -1,  0,  1,  2, -1,  3,  4,  5, -1, -1, -1,  6,  7,
    -1,  8,  9, 10, 11, 12, 13, 14, 15, 16, -1, -1, 17, 18,
    19, -1, 20, 21, 22, -1, 23, 24, -1, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, -1, -1, 39, 40, 41,
    -1, 42, 43, 44, 45, 46, 47, 48, 49, 50, -1, -1, 51, 52,
    53, -1, -1, 54, 55, 56, -1, -1, -1, 57, 58, -1, -1, 59,
    60, 61, -1, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
    -1, 73, 74, -1, 75, -1, 76, 77, -1, -1, -1, 78, 79, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 80, -1, -1, 81,
    -1, 82
  };

const struct KeywordType_t *
KeywordsSet::get_type (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].keyword;

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
