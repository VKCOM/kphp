// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "flex/flex.h"
#include "flex/vk-flex-data.h"

#include <cassert>
#include <cstdio>
#include <cstring>

int node::get_child(unsigned char c, const lang *ctx) const noexcept {
  int l = children_start;
  int r = children_end;
  if (r - l <= 4) {
    for (int i = l; i < r; i++) {
      if (ctx->children[2 * i] == c) {
        return ctx->children[2 * i + 1];
      }
    }
  } else {
    while (r - l > 1) {
      int x = (r + l) >> 1;
      if (ctx->children[2 * x] <= c) {
        l = x;
      } else {
        r = x;
      }
    }
    if (ctx->children[2 * l] == c) {
      return ctx->children[2 * l + 1];
    }
  }
  return -1;
}

bool lang::has_symbol(char c) const noexcept {
  return strchr(flexible_symbols, c) != nullptr;
}

vk::string_view flex(vk::string_view name, vk::string_view case_name, bool is_female, vk::string_view type, int lang_id, char *const dst_buf,
                     char *const err_buf, size_t err_buf_size) {
  if (name.size() > (1 << 10)) {
    snprintf(err_buf, err_buf_size, "Name has length %zu and is too long in function vk_flex", name.size());
    return name;
  }
  if (case_name[0] == 0 || case_name == "Nom") {
    return name;
  }

  if (static_cast<unsigned int>(lang_id) >= static_cast<unsigned int>(LANG_NUM)) {
    snprintf(err_buf, err_buf_size, "Unknown lang id %d in function vk_flex", lang_id);
    return name;
  }

  if (!langs[lang_id]) {
    return name;
  }

  lang *cur_lang = langs[lang_id];

  int start_node = -1;
  if (type == "names") {
    if (cur_lang->names_start < 0) {
      return name;
    }
    start_node = cur_lang->names_start;
  } else if (type == "surnames") {
    if (cur_lang->surnames_start < 0) {
      return name;
    }
    start_node = cur_lang->surnames_start;
  } else {
    snprintf(err_buf, err_buf_size, "Unknown type \"%s\" in function vk_flex", type.data());
    return name;
  }
  assert(start_node >= 0);

  int ca = -1;
  for (int i = 0; i < CASES_NUM && i < cur_lang->cases_num; i++) {
    if (!strcmp(cases_names[i], case_name.data())) {
      ca = i;
      break;
    }
  }
  if (ca == -1) {
    snprintf(err_buf, err_buf_size, "Unknown case \"%s\" in function vk_flex", case_name.data());
    return name;
  }

  int left_pos = 0;
  int wp = 0;
  int name_len = name.size();
  while (left_pos < name_len) {
    int cur_pos = left_pos;
    while (cur_pos < name_len && name[cur_pos] != '-') {
      cur_pos++;
    }
    int hyphen = (name[cur_pos] == '-');
    int best_node = -1;
    int right_pos = cur_pos;
    if (cur_pos - left_pos > 0 && cur_lang->has_symbol(name[cur_pos - 1])) {
      int cur_node = start_node;
      int next_node = -1;
      while (true) {
        assert(cur_node >= 0);
        if (cur_lang->nodes[cur_node].tail_len >= 0) {
          best_node = cur_node;
        }
        if (cur_lang->nodes[cur_node].hyphen >= 0 && hyphen) {
          best_node = cur_lang->nodes[cur_node].hyphen;
        }
        unsigned char c{};
        if (cur_pos == left_pos - 1) {
          break;
        }
        cur_pos--;
        if (cur_pos < left_pos) {
          c = 0;
        } else {
          c = name[cur_pos];
        }
        next_node = cur_lang->nodes[cur_node].get_child(c, cur_lang);
        if (next_node == -1) {
          break;
        } else {
          cur_node = next_node;
        }
      }
    }
    if (best_node == -1) {
      memcpy(dst_buf + wp, name.data() + left_pos, right_pos - left_pos);
      wp += (right_pos - left_pos);
    } else {
      int r = -1;
      if (is_female) {
        r = cur_lang->nodes[best_node].female_endings;
      } else {
        r = cur_lang->nodes[best_node].male_endings;
      }
      if (r < 0 || !cur_lang->endings[r * cur_lang->cases_num + ca]) {
        memcpy(dst_buf + wp, name.data() + left_pos, right_pos - left_pos);
        wp += (right_pos - left_pos);
      } else {
        int ml = right_pos - left_pos - cur_lang->nodes[best_node].tail_len;
        if (ml < 0) {
          ml = 0;
        }
        memcpy(dst_buf + wp, name.data() + left_pos, ml);
        wp += ml;
        strcpy(dst_buf + wp, cur_lang->endings[r * cur_lang->cases_num + ca]);
        wp += static_cast<int>(strlen(cur_lang->endings[r * cur_lang->cases_num + ca]));
      }
    }
    if (hyphen) {
      dst_buf[wp++] = '-';
    } else {
      dst_buf[wp++] = 0;
    }
    left_pos = right_pos + 1;
  }
  dst_buf[wp] = 0;

  return dst_buf;
}
