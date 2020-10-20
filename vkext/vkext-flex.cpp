#include "vkext/vkext-flex.h"

#include <string.h>

#include "flex/vk-flex-data.h"

#include "vkext/vk_zend.h"

#define BUFF_LEN (1 << 16)
static char buff[BUFF_LEN];
extern int verbosity;

char *do_flex(const char *name, int name_len, const char *case_name, int case_name_len, int sex, const char *type, int type_len, int lang_id) {
  if (name_len > (1 << 10)) {
    if (verbosity) {
      fprintf(stderr, "Name is too long\n");
    }
    return estrdup (name);
  }
  lang *cur_lang;
  if (lang_id < 0 || lang_id >= LANG_NUM || !langs[lang_id]) {
    if (verbosity) {
      fprintf(stderr, "Unknown lang id\n");
    }
    return estrdup (name);
  }
  cur_lang = langs[lang_id];
  assert (cur_lang);
  int t = -1;
  if (!strcmp(type, "names")) {
    if (cur_lang->names_start < 0) {
      if (verbosity) {
        fprintf(stderr, "names_start < 0\n");
      }
      return estrdup (name);
    }
    t = cur_lang->names_start;
  } else if (!strcmp(type, "surnames")) {
    if (cur_lang->surnames_start < 0) {
      if (verbosity) {
        fprintf(stderr, "surnames_start < 0\n");
      }
      return estrdup (name);
    }
    t = cur_lang->surnames_start;
  } else {
    if (verbosity) {
      fprintf(stderr, "Unknown type %s\n", type);
    }
    return estrdup (name);
  }
  assert (t >= 0);
  if (sex != 1) {
    sex = 0;
  }
  int ca = -1;
  int i;
  for (i = 0; i < CASES_NUM; i++) {
    if (!strcmp(cases_names[i], case_name)) {
      ca = i;
      break;
    }
  }
  if (ca == -1 || ca >= cur_lang->cases_num) {
    if (verbosity) {
      fprintf(stderr, "Unknown case %s\n", case_name);
    }
    return estrdup (name);
  }
  assert (ca >= 0 && ca < cur_lang->cases_num);
  if (verbosity) {
    fprintf(stderr, "Ok\n");
  }

  int p = 0;
  int wp = 0;
  while (p < name_len) {
    int pp = p;
    while (pp < name_len && name[pp] != '-') {
      pp++;
    }
    int hyphen = (name[pp] == '-');
    int tt = t;
    int best = -1;
    int save_pp = pp;
    int new_tt;
    int isf = 0;
    if (pp - p > 0) {
      const char *fle = cur_lang->flexible_symbols;
      while (*fle) {
        if (*fle == name[pp - 1]) {
          isf = 1;
          break;
        }
        fle++;
      }
    }
    while (1 && isf) {
      if (verbosity) {
        fprintf(stderr, "At node %d\n", tt);
      }
      assert (tt >= 0);
      if (cur_lang->nodes[tt].tail_len >= 0) {
        best = tt;
      }
      if (cur_lang->nodes[tt].hyphen >= 0 && hyphen) {
        best = cur_lang->nodes[tt].hyphen;
      }
      unsigned char c;
      if (pp == p - 1) {
        break;
      }
      pp--;
      if (pp < p) {
        c = 0;
      } else {
        c = name[pp];
      }
      if (verbosity) {
        fprintf(stderr, "Char is %d [%c]\n", (int)c, c ? c : 32);
      }
      new_tt = -1;
      int l = cur_lang->nodes[tt].children_start;
      int r = cur_lang->nodes[tt].children_end;
      if (r - l <= 4) {
        for (i = l; i < r; i++) {
          if (cur_lang->children[2 * i] == c) {
            new_tt = cur_lang->children[2 * i + 1];
            break;
          }
        }
      } else {
        int x;
        while (r - l > 1) {
          x = (r + l) >> 1;
          if (cur_lang->children[2 * x] <= c) {
            l = x;
          } else {
            r = x;
          }
        }
        if (cur_lang->children[2 * l] == c) {
          new_tt = cur_lang->children[2 * l + 1];
        }
      }
      if (new_tt == -1) {
        break;
      } else {
        tt = new_tt;
      }
    }
    if (verbosity) {
      fprintf(stderr, "best = %d\n", best);
      if (best >= 0) {
        fprintf(stderr, "sex = %d\n", sex);
        fprintf(stderr, "tail_len = %d\n", cur_lang->nodes[best].tail_len);
      }
    }
    if (best == -1) {
      memcpy(buff + wp, name + p, save_pp - p);
      wp += (save_pp - p);
    } else {
      int r = -1;
      if (!sex) {
        r = cur_lang->nodes[best].male_endings;
      } else {
        r = cur_lang->nodes[best].female_endings;
      }
      if (verbosity) {
        fprintf(stderr, "save_pp = %d, p = %d, r = %d, ca = %d, cases_num = %d, tail_len = %d\n", save_pp, p, r, ca, cur_lang->cases_num, cur_lang->nodes[best].tail_len);
      }
      if (r < 0 || !cur_lang->endings[r * cur_lang->cases_num + ca]) {
        memcpy(buff + wp, name + p, save_pp - p);
        wp += (save_pp - p);
      } else {
        int ml = save_pp - p - cur_lang->nodes[best].tail_len;
        if (ml < 0) {
          ml = 0;
        }
        memcpy(buff + wp, name + p, ml);
        wp += ml;
        strcpy(buff + wp, cur_lang->endings[r * cur_lang->cases_num + ca]);
        wp += strlen(cur_lang->endings[r * cur_lang->cases_num + ca]);
      }
      if (verbosity) {
        fprintf(stderr, "Name printed\n");
      }
    }
    if (hyphen) {
      buff[wp++] = '-';
    } else {
      buff[wp++] = 0;
    }
    p = save_pp + 1;
  }
  buff[wp] = 0;

  return estrdup (buff);
}
