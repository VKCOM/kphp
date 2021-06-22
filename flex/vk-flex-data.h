#pragma once

struct node {
  short tail_len;
  short hyphen;       // id of hyphen node
  int male_endings;   // offset in lang.endings where male endings starts. First item is:
                      //    lang.endings[male_endings * lang.cases_num]
  int female_endings; // offset in lang.endings where female endings starts
  int children_start; // offset in lang.children where children starts. First item is:
                      //    char, child_id = lang.children[2 * children_start], lang.children[2 * children_start + 1]
  int children_end;   // offset in lang.children where children ends
};

/**
 * Each lang is represented as Prefix Tree containing all patterns (reversed)
 */
struct lang {
  const char *flexible_symbols; // entire alphabet
  int names_start;              // root node id of names Prefix Tree
  int surnames_start;           // root node id of surnames Prefix Tree
  int cases_num;
  const int *children;          // flattened array of edges to children in form [char1, node_id1, char2, node_id2, ...]
  const char **endings;         // flattened array of all rule endings
  node *nodes;                  // array of all nodes in prefix trees
};

extern const int CASES_NUM;
extern const char *cases_names[];
extern const int LANG_NUM;
extern lang *langs[];
