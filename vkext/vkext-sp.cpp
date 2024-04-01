// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/string-processing.h"
#include "common/unicode/unicode-utils.h"

#include "vkext/vkext.h"

PHP_FUNCTION(vk_sp_simplify) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_simplify(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}

PHP_FUNCTION(vk_sp_full_simplify) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_full_simplify(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}

PHP_FUNCTION(vk_sp_deunicode) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_deunicode(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}

PHP_FUNCTION(vk_sp_to_upper) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_to_upper(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}

PHP_FUNCTION(vk_sp_to_lower) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_to_lower(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}

PHP_FUNCTION(vk_sp_sort) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_sort(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}

PHP_FUNCTION(vk_sp_remove_repeats) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_remove_repeats(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}

PHP_FUNCTION(vk_sp_to_cyrillic) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_to_cyrillic(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}

PHP_FUNCTION(vk_sp_words_only) {
  char *s;
  VK_LEN_T len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &len) == FAILURE) {
    return;
  }
  sp_init();
  char *t = sp_words_only(s);
  if (!t) {
    RETURN_EMPTY_STRING();
  } else {
    VK_RETURN_STRING_DUP(t);
  }
}
