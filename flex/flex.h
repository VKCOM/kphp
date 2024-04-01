// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/wrappers/string_view.h"

struct lang;

struct node {
  short tail_len;
  short hyphen;       // id of hyphen node
  int male_endings;   // offset in lang.endings where male endings starts. First item is:
                      //    lang.endings[male_endings * lang.cases_num]
  int female_endings; // offset in lang.endings where female endings starts
  int children_start; // offset in lang.children where children starts. First item is:
                      //    char, child_id = lang.children[2 * children_start], lang.children[2 * children_start + 1]
  int children_end;   // offset in lang.children where children ends

  int get_child(unsigned char c, const lang *ctx) const noexcept;
};

/**
 * Each lang is represented as Prefix Tree containing all patterns (reversed)
 */
struct lang {
  const char *flexible_symbols; // entire alphabet
  int names_start;              // root node id of names Prefix Tree
  int surnames_start;           // root node id of surnames Prefix Tree
  int cases_num;
  const int *children;  // flattened array of edges to children in form [char1, node_id1, char2, node_id2, ...]
  const char **endings; // flattened array of all rule endings
  node *nodes;          // array of all nodes in prefix trees

  bool has_symbol(char c) const noexcept;
};

vk::string_view flex(vk::string_view name, vk::string_view case_name, bool is_female, vk::string_view type, int lang_id, char *dst_buf, char *err_buf,
                     size_t err_buf_size);
