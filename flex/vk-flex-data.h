#pragma once

struct node {
  short tail_len;
  short hyphen;
  int male_endings;
  int female_endings;
  int children_start;
  int children_end;
};

struct lang {
  const char *flexible_symbols;
  int names_start;
  int surnames_start;
  int cases_num;
  const int *children;
  const char **endings;
  node *nodes;
};

extern const int CASES_NUM;
extern const char *cases_names[];
extern const int LANG_NUM;
extern lang *langs[];
