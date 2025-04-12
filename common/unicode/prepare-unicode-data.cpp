// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 500
char s[MAX_LINE_LENGTH];

static void usage_and_exit() {
  fprintf(stderr, "usage: prepare-unicode-data [-c<c1,c2,...>] [-a] <filename> <output-filename>\n"
                  "removes comments and other unnecessary information from unicode data file\n"
                  "[-c<c1,c2,...>]\toutput only specified list of columns\n"
                  "[-a]\tremove angle-bracket comments\n");
  exit(2);
}

#define MAX_COLUMNS 100

int main(int argc, char* const argv[]) {
  int remove_angle_brackets = 0;
  int partial_result = 0;
  char out_columns[MAX_COLUMNS];
  int opt;

  while ((opt = getopt(argc, argv, "ac:")) != -1) {
    switch (opt) {
    case 'a':
      remove_angle_brackets = 1;
      break;
    case 'c': {
      partial_result = 1;
      int x;
      int pos = 0, add;
      memset(out_columns, 0, sizeof(out_columns));
      while (sscanf(optarg + pos, "%d%n", &x, &add) == 1) {
        pos += add;
        assert(0 <= x && x < MAX_COLUMNS);
        out_columns[x] = 1;
        if (optarg[pos] == 0) {
          break;
        }
        assert(optarg[pos] == ',');
        pos++;
      }
      break;
    }
    default:
      printf("Unknown option '%c'", (char)opt);
      usage_and_exit();
    }
  }

  if (optind + 2 != argc) {
    usage_and_exit();
  }

  FILE* in = fopen(argv[optind], "r");
  if (in == NULL) {
    fprintf(stderr, "Can't open file \"%s\"\n", argv[1]);
    return 3;
  }

  FILE* out = fopen(argv[optind + 1], "w");
  if (out == NULL) {
    fprintf(stderr, "Can't open file \"%s\"\n", argv[2]);
    return 4;
  }

  while (fgets(s, MAX_LINE_LENGTH, in)) {
    int l = strlen(s);
    if (s[l - 1] != '\n') {
      fprintf(stderr, "Input file has too long lines\n");
      return 5;
    }

    int i;
    for (i = 0; i < l && s[i] != '#'; i++) {
    }
    while (i > 0 && (s[i - 1] == ' ' || s[i - 1] == '\t' || s[i - 1] == '\r' || s[i - 1] == '\n')) {
      i--;
    }
    int j;
    l = 0;
    for (j = 0; j < i; j++) {
      if ((s[j] != ' ' || (l != 0 && isxdigit(s[j + 1]) && isxdigit(s[l - 1]))) && (s[j] != '0' || (l != 0 && isxdigit(s[l - 1])) || !isxdigit(s[j + 1]))) {
        if (remove_angle_brackets && s[j] == '<') {
          while (s[j] != '>' && j < i) {
            assert(s[j] != ';');
            j++;
          }
          assert(j < i);
          continue;
        }

        s[l++] = s[j];
      }
    }
    while (l > 0 && s[l - 1] == ';') {
      l--;
    }

    if (l) {
      if (partial_result) {
        s[l] = ';';
        s[l + 1] = 0;

        l = 0;
        int k = 0;
        for (i = 0; i < MAX_COLUMNS; i++) {
          char* p = strchr(s + k, ';');
          if (p == NULL) {
            for (; i < MAX_COLUMNS; i++) {
              if (out_columns[i]) {
                l = 0;
              }
            }
            break;
          }
          int new_k = (int)(p - s);
          if (out_columns[i]) {
            if (new_k == k || s[k] == '<') {
              l = 0;
              break;
            }
            for (j = k; j <= new_k; j++) {
              s[l++] = s[j];
            }
          }
          k = new_k + 1;
        }
        while (l > 0 && s[l - 1] == ';') {
          l--;
        }
        if (l == 0) {
          continue;
        }
      }

      s[l++] = '\n';
      s[l] = 0;
      fputs(s, out);
    }
  }

  fclose(in);
  fclose(out);

  return 0;
}
