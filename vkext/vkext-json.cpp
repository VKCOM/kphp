// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "vkext/vkext-json.h"

#include <stdbool.h>

#include "vkext/vkext.h"

static void json_escape_string(const char *s, size_t len);
static bool php_json_encode(zval *val, const char *key);

static int json_determine_array_type(VK_ZVAL_API_P val) {
  int i;
  HashTable *myht = HASH_OF(VK_ZVAL_API_TO_ZVALP(val));

  i = myht ? zend_hash_num_elements(myht) : 0;
  if (i > 0) {
    NEW_INIT_Z_STR_P(key);
    unsigned long int index, idx = 0;
    VK_ZEND_HASH_FOREACH_KEY(myht, index, key) {
      if (VK_ZSTR_P_NON_EMPTY(key) || (index != idx)) {
        return 1;
      }
      idx++;
    }
    VK_ZEND_HASH_FOREACH_END();
  }

  return 0;
}

static bool json_encode_array(VK_ZVAL_API_P val) {
  bool encoded_successfully = true;
  int i, r;
  HashTable *myht;

  if (Z_TYPE_P(VK_ZVAL_API_TO_ZVALP(val)) == IS_ARRAY) {
    myht = HASH_OF(VK_ZVAL_API_TO_ZVALP(val));
    r = json_determine_array_type(val);
  } else {
    myht = Z_OBJPROP_P(VK_ZVAL_API_TO_ZVALP(val));
    r = 1;
  }

  if (r == 0) {
    write_buff_char('[');
  } else {
    write_buff_char('{');
  }

  i = myht ? zend_hash_num_elements(myht) : 0;
  if (i > 0) {
    NEW_INIT_Z_STR_P(key);
    VK_ZVAL_API_P data;
    unsigned long int index;
    int need_comma = 0;

    VK_ZEND_HASH_FOREACH_KEY_VAL(myht, index, key, data) {
      if (r == 0) {
        if (need_comma) {
          write_buff_char(',');
        } else {
          need_comma = 1;
        }

        encoded_successfully = encoded_successfully && php_json_encode(VK_ZVAL_API_TO_ZVALP(data), (key ? key->val : NULL));
      } else if (r == 1) {
        if (key) {
          if (key->val[0] == '\0' && Z_TYPE_P(VK_ZVAL_API_TO_ZVALP(val)) == IS_OBJECT) {
            /* Skip protected and private members. */
            continue;
          }

          if (need_comma) {
            write_buff_char(',');
          } else {
            need_comma = 1;
          }

          json_escape_string(key->val, VK_HASH_KEY_LEN(key->len));
          write_buff_char(':');

          encoded_successfully = encoded_successfully && php_json_encode(VK_ZVAL_API_TO_ZVALP(data), (key ? key->val : NULL));
        } else {
          if (need_comma) {
            write_buff_char(',');
          } else {
            need_comma = 1;
          }

          write_buff_char('"');
          write_buff_long((long)index);
          write_buff_char('"');
          write_buff_char(':');

          encoded_successfully = encoded_successfully && php_json_encode(VK_ZVAL_API_TO_ZVALP(data), (key ? key->val : NULL));
        }
      }
    }
    VK_ZEND_HASH_FOREACH_END();
  }

  if (r == 0) {
    write_buff_char(']');
  } else {
    write_buff_char('}');
  }
  return encoded_successfully;
}

static void json_escape_string(const char *s, size_t len) {
  if (!len) {
    write_buff("\"\"", 2);
    return;
  }

  write_buff_char('"');

  for (size_t pos = 0; pos < len; pos++) {
    char c = s[pos];
    switch (c) {
      case '"':
        write_buff("\\\"", 2);
        break;
      case '\\':
        write_buff("\\\\", 2);
        break;
      case '/':
        write_buff("\\/", 2);
        break;
      case '\b':
        write_buff("\\b", 2);
        break;
      case '\f':
        write_buff("\\f", 2);
        break;
      case '\n':
        write_buff("\\n", 2);
        break;
      case '\r':
        write_buff("\\r", 2);
        break;
      case '\t':
        write_buff("\\t", 2);
        break;
      case 0 ... 7:
      case 11:
      case 14 ... 31:
        break;
      default:
        write_buff_char(c);
        break;
    }
  }

  write_buff_char('"');
}

static bool php_json_encode(zval *val, const char *key) {
  bool encoded_successfully = true;
  HashTable *myht;

  if (VK_ZP_IS_REF_TYPE(val)) {
    VK_ZP_DEREF(val);
  }

  if (UNEXPECTED(Z_TYPE_P(val) == IS_INDIRECT)) {
    val = Z_INDIRECT_P(val);
  }

  switch (Z_TYPE_P(val)) {
    case IS_NULL:
      write_buff("null", 4);
      break;
    case VK_IS_BOOL_T_CASE:
      if (VK_ZVAL_IS_TRUE(val)) {
        write_buff("true", 4);
      } else {
        write_buff("false", 5);
      }
      break;
    case IS_LONG:
      write_buff_long(Z_LVAL_P(val));
      break;
    case IS_DOUBLE: {
      double dbl = Z_DVAL_P(val);

      if (!zend_isinf(dbl) && !zend_isnan(dbl) && dbl < 1e100 && dbl > -1e100) {
        write_buff_double(dbl);
      } else {
        zend_error(E_WARNING, "[json] (php_json_encode) double %.9g does not conform to the JSON spec, encoded as 0", dbl);
        return false;
      }
    } break;
    case IS_STRING:
      json_escape_string(Z_STRVAL_P(val), Z_STRLEN_P(val));
      break;
    case IS_ARRAY:
      myht = HASH_OF(val);
      if (myht && VK_ZEND_HASH_IS_RECURSIVE(myht)) {
        php_error_docref(NULL, E_WARNING, "array recursion detected%s%s", ((key) ? " - " : ""), ((key) ? key : ""));
        write_buff("null", 4);
        break;
      }

      if (myht && VK_ZEND_HASH_APPLY_PROTECTION(myht)) {
        VK_ZEND_HASH_PROTECT_RECURSION(myht);
      }

      encoded_successfully = encoded_successfully && json_encode_array(VK_ZVAL_API_REF(val));

      if (myht && VK_ZEND_HASH_APPLY_PROTECTION(myht)) {
        VK_ZEND_HASH_UNPROTECT_RECURSION(myht);
      }

      break;
    case IS_OBJECT:
      myht = Z_OBJPROP_P(val);
      if (myht && VK_ZEND_HASH_IS_RECURSIVE(myht)) {
        php_error_docref(NULL, E_WARNING, "object recursion detected%s%s", ((key) ? " - " : ""), ((key) ? key : ""));
        write_buff("null", 4);
        break;
      }

      if (myht && VK_ZEND_HASH_APPLY_PROTECTION(myht)) {
        VK_ZEND_HASH_PROTECT_RECURSION(myht);
      }

      encoded_successfully = encoded_successfully && json_encode_array(VK_ZVAL_API_REF(val));

      if (myht && VK_ZEND_HASH_APPLY_PROTECTION(myht)) {
        VK_ZEND_HASH_UNPROTECT_RECURSION(myht);
      }

      break;
    default:
      zend_error(E_WARNING, "[json] (php_json_encode) type is unsupported, encoded as null");
      write_buff("null", 4);
      break;
  }

  return encoded_successfully;
}

bool vk_json_encode(zval *val) {
  return php_json_encode(val, NULL);
}
