// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <fcntl.h>
#include <zlib.h>

#include "vkext/vkext-rpc-include.h"
#include "vkext/vkext-rpc.h"
#include "vkext/vkext-tl-parse.h"
#include "vkext/vkext.h"

#define MEMCACHE_ERROR 0x7ae432f5
#define MEMCACHE_VALUE_NOT_FOUND 0x32c42422
#define MEMCACHE_VALUE_LONG 0x9729c42
#define MEMCACHE_VALUE_STRING 0xa6bebb1a
#define MEMCACHE_FALSE 0xbc799737
#define MEMCACHE_TRUE 0x997275b5

static char error_buf[1000];

struct memcache_value {
  unsigned int type;
  union {
    long long lval;
    struct {
      int len;
      char *data;
    } strval;
  } val;
  int flags;
};

static int mmc_uncompress(char **result, unsigned long *result_len, const char *data, int data_len) /* {{{ */
{
  int status;
  unsigned int factor = 1, maxfactor = 16;
  char *tmp1 = NULL;

  do {
    *result_len = (unsigned long)data_len * (1 << factor++);
    *result = (char *)erealloc(tmp1, *result_len);
    status = uncompress((unsigned char *)*result, result_len, (unsigned const char *)data, data_len);
    tmp1 = *result;
  } while (status == Z_BUF_ERROR && factor < maxfactor);

  if (status == Z_OK) {
    *result = static_cast<char *>(erealloc(*result, *result_len + 1));
    (*result)[*result_len] = '\0';
    return 1;
  }

  efree(*result);
  return 0;
}

/* }}}*/

/* }}}*/

struct memcache_value do_fetch_memcache_value() {
  memcache_value value;
  auto x = static_cast<unsigned int>(tl_parse_int());
  switch (x) {
    case MEMCACHE_VALUE_NOT_FOUND:
      value.type = MEMCACHE_VALUE_NOT_FOUND;
      break;
    case MEMCACHE_VALUE_LONG:
      value.type = MEMCACHE_VALUE_LONG;
      value.val.lval = tl_parse_long();
      value.flags = tl_parse_int();
      break;
    case MEMCACHE_VALUE_STRING:
      value.type = MEMCACHE_VALUE_STRING;
      value.val.strval.len = tl_parse_string(&value.val.strval.data);
      value.flags = tl_parse_int();
      break;
    case MEMCACHE_ERROR:
      value.type = MEMCACHE_ERROR;
      tl_parse_long();
      tl_parse_int();
      value.val.strval.len = tl_parse_string(&value.val.strval.data);
      break;
    case MEMCACHE_TRUE:
      value.type = MEMCACHE_TRUE;
      break;
    case MEMCACHE_FALSE:
      value.type = MEMCACHE_FALSE;
      break;
    default:
      value.type = MEMCACHE_FALSE;
      snprintf(error_buf, 1000, "Unknown magic %x", x);
      tl_set_error(error_buf);
      break;
  }
  return value;
}

static int mmc_postprocess_value(zval **return_value, char *value, int value_len) {
  const char *value_tmp = value;
  php_unserialize_data_t var_hash;
  PHP_VAR_UNSERIALIZE_INIT(var_hash);

  if (!php_var_unserialize(Z_PP_TO_ZAPI_P(return_value), (const unsigned char **)&value_tmp, (const unsigned char *)(value_tmp + value_len), &var_hash)) {
    ZVAL_FALSE(*return_value);
    PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
    php_error_docref(NULL, E_NOTICE, "unable to unserialize data");
    return 0;
  }

  PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
  return 1;
}

void php_rpc_fetch_memcache_value(INTERNAL_FUNCTION_PARAMETERS) {
  struct memcache_value value = do_fetch_memcache_value();
  switch (value.type) {
    case MEMCACHE_VALUE_LONG:
      RETURN_LONG(value.val.lval);
      break;
    case MEMCACHE_VALUE_STRING:
      if (value.flags & 2) {
        char *res = 0;
        unsigned long res_len = 0;
        if (!mmc_uncompress(&res, &res_len, value.val.strval.data, value.val.strval.len)) {
          RETURN_FALSE;
        }

        if (value.flags & 1) {
          mmc_postprocess_value(&return_value, res, res_len);
          efree(res);
        } else {
          VK_RETURN_STRINGL_NOD(res, res_len);
        }
        break;
      } else {
        if (value.flags & 1) {
          mmc_postprocess_value(&return_value, value.val.strval.data, value.val.strval.len);
        } else {
          ADD_RMALLOC(value.val.strval.len + 1);
          VK_RETURN_STRINGL_DUP(value.val.strval.data, value.val.strval.len);
        }
        break;
      }
    case MEMCACHE_ERROR:
    case MEMCACHE_FALSE:
    case MEMCACHE_VALUE_NOT_FOUND:
    default:
      RETURN_FALSE;
  }
}
