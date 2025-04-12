// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 500
char s[MAX_LINE_LENGTH];

#define PREPARE_TABLE_SIZE 0x500
#define PREPARE_TABLE_SIZE_STR "1280"

void read_ucf (const char *filename, int ucf[0x110000][4]) {
  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "Can't open file \"%s\"\n", filename);
    exit (1);
  }

  while (fgets (s, MAX_LINE_LENGTH, f)) {
    int l = strlen (s);
    assert (l > 0 && s[l - 1] == '\n');

    int c1, c2;
    char type;
    int pos;
    assert (sscanf (s, "%x;%c;%x%n", &c1, &type, &c2, &pos) == 3);
    assert (0 < c1 && c1 < 0x110000);
    assert (0 < c2 && c2 < 0x110000);
    assert (c1 != c2);
    switch (type) {
      case 'F':
        if ((0 && c1 < PREPARE_TABLE_SIZE) ||
            (0x1f80 <= c1 && c1 <= 0x1ffc) || 1) {
//          fprintf (stderr, "Ignored %x\n", c1);
          break;
        } else {
          int add = 0, i = 1;
          while (i < 3 && sscanf (s + pos, "%x%n", &ucf[c1][i], &add) == 1) {
            assert (0 < ucf[c1][i] && ucf[c1][i] < 0x110000);
            pos += add;
            i++;
          }
          assert (i <= 3);
        }
        //fall through
      case 'S':
        //fall through
      case 'C':
        if (type == 'F' || !ucf[c1][0]) {
          ucf[c1][0] = c2;
        }
        //fall through
      case 'T':
        //ignored
        assert (pos + 1 == l);
        break;
    }
  }

  fclose (f);
}

void read_unfkd (const char *filename, int ucf[0x110000][19]) {
  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "Can't open file \"%s\"\n", filename);
    exit (1);
  }

  while (fgets (s, MAX_LINE_LENGTH, f)) {
    int l = strlen (s);
    assert (l > 0 && s[l - 1] == '\n');

    int c;
    int pos, add;
    assert (sscanf (s, "%x%n", &c, &pos) == 1);
    assert (0 < c && c < 0x110000);
    if (s[pos] == '\n') {
      ucf[c][0] = 0;
    } else {
      assert (s[pos] == ';');
      pos++;
      int i = 0;
      while (i < 18 && sscanf (s + pos, "%x%n", &ucf[c][i], &add) == 1) {
        assert (0 < ucf[c][i] && ucf[c][i] < 0x110000);
        pos += add;
        i++;
      }
      assert (i <= 18);
      assert (pos + 1 == l);
    }
  }

  fclose (f);
}

void read_unfkc_cf (const char *filename, int ucf[0x110000][19]) {
  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "Can't open file \"%s\"\n", filename);
    exit (1);
  }
  int j;
  for (j = 0; j < 0x110000; j++) {
    ucf[j][0] = j;
  }

  while (fgets (s, MAX_LINE_LENGTH, f)) {
    int l = strlen (s);
    assert (l > 0 && s[l - 1] == '\n');

    int c1, c2;
    int pos, add;
    assert (sscanf (s, "%x%n", &c1, &pos) == 1);
    if (s[pos] == '.') {
      assert (s[pos + 1] == '.');
      assert (sscanf (s + pos + 2, "%x%n", &c2, &add) == 1);
      pos += add + 2;
    } else {
      c2 = c1;
    }
    assert (0 < c1 && c1 <= c2 && c2 < 0x110000);
    if (s[pos] == '\n') {
      for (j = c1; j <= c2; j++) {
        ucf[j][0] = 0;
      }
    } else {
      assert (s[pos] == ';');
      pos++;
      int i = 0;
      while (i < 18 && sscanf (s + pos, "%x%n", &ucf[c1][i], &add) == 1) {
        assert (0 < ucf[c1][i] && ucf[c1][i] < 0x110000);
        for (j = c1 + 1; j <= c2; j++) {
          ucf[j][i] = ucf[c1][i];
        }
        pos += add;
        i++;
      }
      assert (i <= 18);
      assert (pos + 1 == l);
    }
  }

  fclose (f);
}

#define Cn 0  //Unassigned
#define Lu 1  //Uppercase_Letter
#define Ll 2  //Lowercase_Letter
#define Lt 3  //Titlecase_Letter
#define Lm 4  //Modifier_Letter
#define Lo 5  //Other_Letter
#define Mn 6  //Nonspacing_Mark
#define Me 7  //Enclosing_Mark
#define Mc 8  //Spacing_Mark
#define Nd 9  //Decimal_Number
#define Nl 10 //Letter_Number
#define No 11 //Other_Number
#define Zs 12 //Space_Separator
#define Zl 13 //Line_Separator
#define Zp 14 //Paragraph_Separator
#define Cc 15 //Control
#define Cf 16 //Format
#define Co 17 //Private_Use
#define Cs 18 //Surrogate
#define Pd 19 //Dash_Punctuation
#define Ps 20 //Open_Punctuation
#define Pe 21 //Close_Punctuation
#define Pc 22 //Connector_Punctuation
#define Po 23 //Other_Punctuation
#define Sm 24 //Math_Symbol
#define Sc 25 //Currency_Symbol
#define Sk 26 //Modifier_Symbol
#define So 27 //Other_Symbol
#define Pi 28 //Initial_Punctuation
#define Pf 29 //Final_Punctuation

#define symbol ((1 << Cn) | (1 << Lu) | (1 << Ll) | (1 << Lt) | (1 << Lm) | (1 << Lo) | (1 << Nd) | (1 << Nl) | (1 << No) | (1 << Co))
#define is_symbol(x) ((symbol >> ugc[(x)]) & 1)
#define modifier ((1 << Mn) | (1 << Me) | (1 << Mc) | (1 << Cf) | (1 << Cs))
#define is_modifier(x) ((modifier >> ugc[(x)]) & 1)

void read_ugc (const char *filename, int ugc[0x110000]) {
  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "Can't open file \"%s\"\n", filename);
    exit (1);
  }
  memset (ugc, -1, sizeof (ugc[0]) * 0x110000);

  while (fgets (s, MAX_LINE_LENGTH, f)) {
    int l = strlen (s);
    assert (l > 0 && s[l - 1] == '\n');

    int c1, c2;
    int pos, add;
    assert (sscanf (s, "%x%n", &c1, &pos) == 1);
    if (s[pos] == '.') {
      assert (s[pos + 1] == '.');
      assert (sscanf (s + pos + 2, "%x%n", &c2, &add) == 1);
      pos += add + 2;
    } else {
      c2 = c1;
    }
    assert (s[pos] == ';');
    assert (pos + 4 == l);
    assert (0 <= c1 && c1 < 0x110000);
    assert (0 <= c2 && c2 < 0x110000);
    assert ('A' <= s[pos + 1] && 'Z' >= s[pos + 1]);
    assert ('a' <= s[pos + 2] && 'z' >= s[pos + 2]);

    int type = -1;
    switch (s[pos + 1] * 128 + s[pos + 2]) {
      case 'C' * 128 + 'n':
        type = Cn;
        break;
      case 'L' * 128 + 'u':
        type = Lu;
        break;
      case 'L' * 128 + 'l':
        type = Ll;
        break;
      case 'L' * 128 + 't':
        type = Lt;
        break;
      case 'L' * 128 + 'm':
        type = Lm;
        break;
      case 'L' * 128 + 'o':
        type = Lo;
        break;
      case 'M' * 128 + 'n':
        type = Mn;
        break;
      case 'M' * 128 + 'e':
        type = Me;
        break;
      case 'M' * 128 + 'c':
        type = Mc;
        break;
      case 'N' * 128 + 'd':
        type = Nd;
        break;
      case 'N' * 128 + 'l':
        type = Nl;
        break;
      case 'N' * 128 + 'o':
        type = No;
        break;
      case 'Z' * 128 + 's':
        type = Zs;
        break;
      case 'Z' * 128 + 'l':
        type = Zl;
        break;
      case 'Z' * 128 + 'p':
        type = Zp;
        break;
      case 'C' * 128 + 'c':
        type = Cc;
        break;
      case 'C' * 128 + 'f':
        type = Cf;
        break;
      case 'C' * 128 + 'o':
        type = Co;
        break;
      case 'C' * 128 + 's':
        type = Cs;
        break;
      case 'P' * 128 + 'd':
        type = Pd;
        break;
      case 'P' * 128 + 's':
        type = Ps;
        break;
      case 'P' * 128 + 'e':
        type = Pe;
        break;
      case 'P' * 128 + 'c':
        type = Pc;
        break;
      case 'P' * 128 + 'o':
        type = Po;
        break;
      case 'S' * 128 + 'm':
        type = Sm;
        break;
      case 'S' * 128 + 'c':
        type = Sc;
        break;
      case 'S' * 128 + 'k':
        type = Sk;
        break;
      case 'S' * 128 + 'o':
        type = So;
        break;
      case 'P' * 128 + 'i':
        type = Pi;
        break;
      case 'P' * 128 + 'f':
        type = Pf;
        break;
      default:
        fprintf (stderr, "Unknown general category %c%c\n", s[pos + 1], s[pos + 2]);
        assert (0);
    }

    int i;
    for (i = c1; i <= c2; i++) {
      ugc[i] = type;
    }
  }

  fclose (f);
}

void read_diacritics (const char *filename, int is_diacritic[0x110000]) {
  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "Can't open file \"%s\"\n", filename);
    exit (1);
  }

  while (fgets (s, MAX_LINE_LENGTH, f)) {
    int l = strlen (s);
    assert (l > 0 && s[l - 1] == '\n');

    int c1, c2;
    int pos, add;
    assert (sscanf (s, "%x%n", &c1, &pos) == 1);
    if (s[pos] == '.') {
      assert (s[pos + 1] == '.');
      assert (sscanf (s + pos + 2, "%x%n", &c2, &add) == 1);
      pos += add + 2;
    } else {
      c2 = c1;
    }
    assert (pos + 1 == l);

    int i;
    for (i = c1; i <= c2; i++) {
      is_diacritic[i] = 1;
    }
  }

  fclose (f);
}


void read_case (const char *filename, int to_case[0x110000]) {
  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "Can't open file \"%s\"\n", filename);
    exit (1);
  }

  int j;
  for (j = 0; j < 0x110000; j++) {
    to_case[j] = j;
  }

  while (fgets (s, MAX_LINE_LENGTH, f)) {
    int l = strlen (s);
    assert (l > 0 && s[l - 1] == '\n');

    int c1, c2;
    int pos;
    assert (sscanf (s, "%x;%x%n", &c1, &c2, &pos) == 2);
    assert (pos + 1 == l);

    to_case[c1] = c2;
  }

  fclose (f);
}

int ranges[0x110000][2];

void output_table (FILE *out, const char *name, int *data, int int_table) {
  fprintf (out, "static %s %s[TABLE_SIZE] = {\n", int_table ? "int" : "short", name);

  int i, size = PREPARE_TABLE_SIZE;
  for (i = 0; i < size / 16; i++) {
    fprintf (out, "    ");
    int j;
    for (j = 0; j < 16; j++) {
      assert (0 <= data[i * 16 + j] && data[i * 16 + j] <= (int_table ? 0x10ffff : 0x7fff));
      fprintf (out, "%5d, ", data[i * 16 + j]);
    }
    if (i + 1 == size / 16) {
      fprintf (out, "};\n\n");
    } else {
      fprintf (out, "\n");
    }
  }

  int cur_mask = 63, cur_diff = data[PREPARE_TABLE_SIZE] - PREPARE_TABLE_SIZE, cur_target = data[PREPARE_TABLE_SIZE], pos = 0;
  ranges[pos][0] = PREPARE_TABLE_SIZE;
  for (i = PREPARE_TABLE_SIZE; i <= 0x110000; i++) {
    int mask = 0;
    int diff = 0;
    if (i < 0x110000) {
      assert (0 <= data[i] && data[i] <= 0x10ffff);

      diff = data[i] - i;
      if (diff == cur_diff) {
        mask |= 1;
      }
      if (data[i] == (i & -2)) {
        mask |= 2;
      }
      if (data[i] == (i | 1)) {
        mask |= 4;
      }
      if (data[i] == ((i - 1) | 1)) {
        mask |= 8;
      }
      if (data[i] == cur_target) {
        mask |= 64;
      }
    }

    if (mask & cur_mask) {
      cur_mask &= mask;
    } else {
      if (cur_mask & 64) {
        ranges[pos][1] = cur_target;//>= 0 - to x
      } else if (cur_mask & 1) {
        ranges[pos][1] = ~(ranges[pos][0] + cur_diff); //< 0 into range beginning with ~x
      } else if (cur_mask & 2) {
        ranges[pos][1] = 0x200000;
      } else if (cur_mask & 4) {
        ranges[pos][1] = 0x200001;
      } else if (cur_mask & 8) {
        ranges[pos][1] = 0x200002;
      } else {
        assert (0);
      }
      cur_mask = (mask | 1 | 64);
      cur_diff = diff;
      cur_target = data[i];
      pos++;
      ranges[pos][0] = i;
    }
    if (cur_mask & 1) {
      assert (diff == cur_diff);
    }
  } 
  ranges[pos][0] = INT_MAX;
  ranges[pos][1] = 0;
  pos++;

  fprintf (out, "#define %s_ranges_size %d\n", name, pos * 2);
  fprintf (out, "static const int %s_ranges[%s_ranges_size] = {\n", name, name);
  for (i = 0; i < pos; i++) {
    fprintf (out, "  %d, %d", ranges[i][0], ranges[i][1]);
    if (i + 1 == pos) {
      fprintf (out, "};\n\n");
    } else {
      fprintf (out, ",\n");
    }
  }
//  fprintf (stderr, "Table %s size = %d\n", name, pos);
}


int ucf[0x110001][4];
int unfkd[0x110001][19];
int ugc[0x110001];
int convert_to[0x110001][19];
int len[0x110001];
int without_diacritics[0x110001];
int to_upper[0x110001];
int to_lower[0x110001];


int main (int argc, const char *argv[]) {
  if (argc != 7) {
    fprintf (stderr, "usage: generate-unicode-functions <unicode-case-folding-filename> <unicode-general-category-filename> <unicode-NFKD-filename> <unicode-uppercase-filename> <unicode-lowercase-filename> <output-filename>\n"
                     "generates prepare_search_string and other functions depending on unicode data\n");
    return 2;
  }

  read_ucf (argv[1], ucf);
  read_ugc (argv[2], ugc);
  read_unfkd (argv[3], unfkd);
  read_case (argv[4], to_upper);
  read_case (argv[5], to_lower);

  int i, j;

  for (i = 0; i < 0x110000; i++) {
    if (unfkd[i][0] != 0 && unfkd[i][1] == 0 && 0x4E00 <= unfkd[i][0] && unfkd[i][0] <= 0x9FCC && !is_symbol (i)) {
      ugc[i] = Lo;//fixing CJK ideographs
    }
    if (unfkd[i][0] != 0 && unfkd[i][1] == 0 && is_symbol (unfkd[i][0]) && !is_symbol (i)) {
      ugc[i] = Lo;//fixing other not symbols like symbols
//      fprintf (stderr, "fixing %x\n", i);
    }
  }
  ugc[0x2103] = ugc[0x2109] = Lo;

  //add some diacritics
  ugc[0xb7] = ugc[0x2bc] = ugc[0x2be] = ugc[0x30fc] = Mc;

  for (i = 0; i < 0x110000; i++) {
    if (ucf[i][0] && !is_symbol (i))	 {
      if (i == 0x345) {
        ugc[i] = Lo;
        ucf[i][0] = 0;
      } else if (0x24b6 <= i && i <= 0x24cf) {
        ugc[ucf[i][0]] = Lo;
        ugc[i] = Lo;
      } else {
        fprintf (stderr, "Error3 in %x(%d): %x(%d)\n", i, ugc[i], ucf[i][0], ugc[ucf[i][0]]);
        assert (0);
      }
    }
  }
  for (i = 0; i < 0x110000; i++) {
    int cnt_s = 0, pos = 0, cnt_o = 0;
    for (j = 0; unfkd[i][j]; j++) {
      if (is_symbol (unfkd[i][j])) {
        if (unfkd[i][j] != 0x345) {
          cnt_s++;
          pos = j;
        }
      } else if (!is_modifier (unfkd[i][j])) {
        cnt_o++;
        if (cnt_s == 0) {
          pos = j;
        }
      }
    }

    if (cnt_s + cnt_o == 1 || (cnt_s == 1 && cnt_o == 1)) {
      without_diacritics[i] = unfkd[i][pos];
    } else {
      if (j > 0 && is_symbol (i)) {
//        fprintf (stderr, "Error4 in %x(%d), cnt_s = %d, cnt_m = %d, cnt_o = %d\n", i, ugc[i], cnt_s, j - cnt_o - cnt_s, cnt_o);
      }
      without_diacritics[i] = i;
    }
  }

  for (i = 0; i < 0x110000; i++) {
    j = i;
    int cnt = 0;
    while (j != without_diacritics[j]) {
      j = without_diacritics[j];
      cnt++;
      assert (cnt < 3);
    }

    assert (is_modifier (j) == is_modifier (i));
    without_diacritics[i] = is_modifier (j) ? 0 : j;
  }

  for (i = 0; i < 0x110000; i++) {
    int k = 0;
    for (j = 0; ucf[i][j]; j++) {
      if (is_symbol (ucf[i][j])) {
        ucf[i][k++] = ucf[i][j];
      } else if (!is_modifier (ucf[i][j])) {
        if (k == 0 || ucf[i][k - 1] != ' ') {
          ucf[i][k++] = ' ';
        }
      }
    }
    if (!(j == 0 || is_symbol (i))) {
      fprintf (stderr, "Error1 in %x(%d)\n", i, ugc[i]);
      assert (0);
    }
    if (j > 0 && k == 0) {
      fprintf (stderr, "Error2 in %x(%d)\n", i, ugc[i]);
      assert (0);
    }
    ucf[i][k] = 0;
  }

  for (i = 1; i < 0x110000; i++) {
    if (is_symbol (i)) {
      if (ucf[i][0]) {
        for (j = 0; ucf[i][j]; j++) {
          convert_to[i][j] = ucf[i][j];
        }
        len[i] = j;
      } else {
        int ti = without_diacritics[i];
        if (ucf[ti][0]) {
          for (j = 0; ucf[ti][j]; j++) {
            convert_to[i][j] = ucf[ti][j];
          }
          len[i] = j;
        } else {
          convert_to[i][0] = i;
          len[i] = 1;
        }
      }
    } else if (is_modifier (i)) {
      len[i] = 0;
    } else {
      convert_to[i][0] = ' ';
      len[i] = 1;
    }
    if (len[i] > 1) {
      if (i == 0xdf && i < PREPARE_TABLE_SIZE) {
        convert_to[i][0] = i;
        len[i] = 1;

        ucf[0x1E9E][0] = i;
        ucf[0x1E9E][1] = 0;
      } else if (i == 0x149 && i < PREPARE_TABLE_SIZE) {
        convert_to[i][0] = 0x6e;
        len[i] = 1;
      } else if (i == 0x587 && i < PREPARE_TABLE_SIZE) {
        convert_to[i][0] = i;
        len[i] = 1;
      } else {
        assert (i >= PREPARE_TABLE_SIZE);
        fprintf (stderr, "Long: %x %d: %x(%d) %x(%d)\n", i, len[i], convert_to[i][0], ugc[convert_to[i][0]], convert_to[i][1], ugc[convert_to[i][1]]);
      }
    }
  }

  //remove non-characters
  for (i = 0xFDD0; i <= 0xFDEF; i++) {
    convert_to[i][0] = ' ';
    len[i] = 1;
  }
  for (i = 0; i <= 16; i++) {
    convert_to[(i << 16) + 0xFFFE][0] = ' ';
    len[(i << 16) + 0xFFFE] = 1;

    convert_to[(i << 16) + 0xFFFF][0] = ' ';
    len[(i << 16) + 0xFFFF] = 1;
  }


  for (i = 0; i < 0x110000; i++) {
    assert (len[i] <= 1);
    int ti = without_diacritics[i];
    if (ti != i) {
      if (without_diacritics[convert_to[i][0]] != without_diacritics[convert_to[ti][0]]) {
        if (i != 0x3f9) {
          fprintf (stderr, "%x/%d(%x %x) %x/%d(%x %x)\n", i, i, convert_to[i][0], convert_to[i][1], ti, ti, convert_to[ti][0], convert_to[ti][1]);
          assert (0);
        }
      }
    }
  }

  assert ((PREPARE_TABLE_SIZE & 15) == 0);
  
  FILE *out = fopen (argv[6], "w");
  if (out == NULL) {
    fprintf (stderr, "Can't open file \"%s\"\n", argv[6]);
    return 1;
  }

  fputs ("#define TABLE_SIZE " PREPARE_TABLE_SIZE_STR "\n"
         "\n"
         , out);

  for (i = 0; i < 0x110000; i++) {
    assert (len[i] <= 1);
    len[i] = convert_to[i][0];
  }
  
  output_table (out, "prepare_table", len, 0);

  output_table (out, "to_upper_table", to_upper, 1);

  output_table (out, "to_lower_table", to_lower, 1);

/*
  fputs ("/ * Search generated ranges for specified character * /\n"
         "static int binary_search_ranges (const int *ranges, int r, int c) {\n"
         "  if ((unsigned int)c > 0x10ffff) {\n"
         "    return 0;\n"
         "  }\n"
         "  \n"
         "  int l = 0;\n"
         "  while (l + 2 < r) {\n"
         "    int m = ((l + r) >> 2) << 1;\n"
         "    if (ranges[m] <= c) {\n"
         "      l = m;\n"
         "    } else {\n"
         "      r = m;\n"
         "    }\n"
         "  }\n"
         "  \n"
         "  int t = ranges[l + 1];\n"
         "  if (t < 0) {\n"
         "    return c - ranges[l] + (~t);\n"
         "  }\n"
         "  if (t <= 0x10ffff) {\n"
         "    return t;\n"
         "  }\n"
         "  switch (t - 0x200000) {\n"
         "    case 0:\n"
         "      return (c & -2);\n"
         "    case 1:\n"
         "      return (c | 1);\n"
         "    case 2:\n"
         "      return ((c - 1) | 1);\n"
         "    default:\n"
         "      assert (0);\n"
         "      exit (1);\n"
         "  }\n"
         "}\n\n",
         out);

  fputs ("/ * Removes diacritics from unicode symbol * /\n"
         "int remove_diacritics (int c) {\n"
         "  if ((unsigned int)c < (unsigned int)TABLE_SIZE) {\n"
         "    return without_diacritics[c];\n"
         "  } else {\n"
         "    return binary_search_ranges (without_diacritics_ranges, without_diacritics_ranges_size, c);\n"
         "  }\n"
         "}\n\n",
         out);

  fputs ("/ * Prepares unicode character for search * /\n"
         "int prepare_search_character (int c) {\n"
         "  if ((unsigned int)c < (unsigned int)TABLE_SIZE) {\n"
         "    return prepare_table[c];\n"
         "  } else {\n"
         "    return binary_search_ranges (prepare_table_ranges, prepare_table_ranges_size, c);\n"
         "  }\n"
         "}\n\n",
         out);

  fputs ("/ * Prepares unicode 0-terminated string input for search,\n"
         "   leaving only digits and letters with diacritics.\n"
         "   Length of string can decrease.\n"
         "   Returns length of result. * /\n"
         "int prepare_search_string (int *input) {\n"
         "  int i;\n"
         "  int *output = input;\n"
         "  for (i = 0; input[i]; i++) {\n"
         "    int c = input[i], new_c;\n"
         "    if ((unsigned int)c < (unsigned int)TABLE_SIZE) {\n"
         "      new_c = prepare_table[c];\n"
         "    } else {\n"
         "      new_c = binary_search_ranges (prepare_table_ranges, prepare_table_ranges_size, c);\n"
         "    }\n"
         "    if (new_c) {\n"
         "      if (new_c != 0x20 || (output > input && output[-1] != 0x20)) {\n"
         "        *output++ = new_c;\n"
         "      }\n"
         "    }\n"
         "  }\n"
         "  if (output > input && output[-1] == 0x20) {\n"
         "    output--;\n"
         "  }\n"
         "  *output = 0;\n"
         "  return output - input;\n"
         "}\n\n",
         out);

  fputs ("/ * Differs prepare_search_* behaviour, forcing\n"
         "   replace of character 'from' by character 'to'. * /\n"
         "void add_prepare_search_exception (int from, int to) {\n"
         "  assert ((unsigned int)from < (unsigned int)TABLE_SIZE);\n"
         "  assert (0 <= to && to <= 0x7fff);\n"
         "  prepare_table[from] = to;\n"
         "}\n\n",
         out);
*/
  
  fclose (out);
  return 0;
}
