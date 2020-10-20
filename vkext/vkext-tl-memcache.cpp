#include <fcntl.h>
#include <zlib.h>

#include "common/algorithms/find.h"

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
#define MEMCACHE_SET 0xeeeb54c4u
#define MEMCACHE_ADD 0xa358f31cu
#define MEMCACHE_REPLACE 0x2ecdfaa2u
#define MEMCACHE_INCR 0x80e6c950u
#define MEMCACHE_DECR 0x6467e0d9u
#define MEMCACHE_DELETE 0xab505c0a
#define MEMCACHE_GET 0xd33b13ae

#define TL_ENGINE_MC_SET_QUERY 0x3bd91922u
#define TL_ENGINE_MC_ADD_QUERY 0xd02f7d07u
#define TL_ENGINE_MC_REPLACE_QUERY 0x5dc86a99u
#define TL_ENGINE_MC_DELETE_QUERY 0x828688dc
#define TL_ENGINE_MC_INCR_QUERY 0xcadd63e1u
#define TL_ENGINE_MC_DECR_QUERY 0x71f416e9u
#define TL_ENGINE_MC_GET_QUERY 0x62408e9e


#define COMPRESS_LEVEL 4
#define COMPRESS_MIN_SAVE 10.7

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

long long do_memcache_send_get(struct rpc_connection *c, const char *key, int key_len, double timeout, int wait, int fake) {
  do_rpc_clean();
  do_rpc_store_int(fake ? TL_ENGINE_MC_GET_QUERY : MEMCACHE_GET);
  do_rpc_store_string(key, key_len);
  struct rpc_query *q;
  if (!(q = do_rpc_send_noflush(c, timeout, !wait))) {
    return -1;
  }
  if (q == (struct rpc_query *)1) { // answer is ignored
    return 0;
  }
  assert (wait);
  return q->qid;
}

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

static int mmc_compress(char **result, unsigned long *result_len, const char *data, int data_len) /* {{{ */
{
  int status, level = COMPRESS_LEVEL;

  *result_len = data_len + (data_len / 1000) + 25 + 1; /* some magic from zlib.c */
  *result = (char *)emalloc(*result_len);

  if (!*result) {
    return 0;
  }

  if (level >= 0) {
    status = compress2((unsigned char *)*result, result_len, (unsigned const char *)data, data_len, level);
  } else {
    status = compress((unsigned char *)*result, result_len, (unsigned const char *)data, data_len);
  }

  if (status == Z_OK) {
    *result = static_cast<char *>(erealloc(*result, *result_len + 1));
    (*result)[*result_len] = '\0';
    return 1;
  }

  efree(*result);
  return 0;
}

/* }}}*/

long long do_memcache_send_store(struct rpc_connection *c, const char *key, int key_len, const char *value, int value_len, int flags, int delay, int op, double timeout, int wait) {

  char *data = 0;
  if (flags & 2) {
    unsigned long data_len;

    if (!mmc_compress(&data, &data_len, value, value_len)) {
      return -1;
    }

    /* was enough space saved to motivate uncompress processing on get */
    if (data_len < value_len * COMPRESS_MIN_SAVE) {
      value = data;
      value_len = data_len;
    } else {
      flags &= ~2;
      efree(data);
      data = NULL;
    }
  }
  do_rpc_clean();
  do_rpc_store_int(op);
  do_rpc_store_string(key, key_len);
  do_rpc_store_int(flags);
  do_rpc_store_int(delay);
  do_rpc_store_string(value, value_len);
  if (data) {
    efree (data);
  }
  struct rpc_query *q;
  if (!(q = do_rpc_send_noflush(c, timeout, !wait))) {
    return -1;
  }
  if (q == (struct rpc_query *)1) { // answer is ignored
    return 0;
  }
  assert (wait);
  return q->qid;
}

long long do_memcache_send_incr(struct rpc_connection *c, const char *key, int key_len, long long val, int op, double timeout, int wait) {
  do_rpc_clean();
  do_rpc_store_int(op);
  do_rpc_store_string(key, key_len);
  do_rpc_store_long(val);
  struct rpc_query *q;
  if (!(q = do_rpc_send_noflush(c, timeout, !wait))) {
    return -1;
  }
  if (q == (struct rpc_query *)1) { // answer is ignored
    return 0;
  }
  assert (wait);
  return q->qid;
}

long long do_memcache_send_delete(struct rpc_connection *c, const char *key, int key_len, double timeout, int wait, int fake) {
  do_rpc_clean();
  do_rpc_store_int(fake ? TL_ENGINE_MC_DELETE_QUERY : MEMCACHE_DELETE);
  do_rpc_store_string(key, key_len);
  struct rpc_query *q;
  if (!(q = do_rpc_send_noflush(c, timeout, !wait))) {
    return -1;
  }
  if (q == (struct rpc_query *)1) { // answer is ignored
    return 0;
  }
  assert (wait);
  return q->qid;
}

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

struct memcache_value do_memcache_parse_value() {
  tl_parse_init();
  struct memcache_value value = do_fetch_memcache_value();
  tl_parse_end();
  if (tl_parse_error()) {
    value.type = MEMCACHE_ERROR;
    value.val.strval.data = tl_parse_error();
    value.val.strval.len = strlen(value.val.strval.data);
  }
  return value;
}

struct memcache_value do_memcache_act(struct rpc_connection *c, long long qid, int flags, double timeout) {
  struct memcache_value value;
  if (qid < 0) {
    value.type = MEMCACHE_ERROR;
    value.val.strval.data = global_error;
    value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
    return value;
  }
  if ((flags & 1) && /*do_rpc_flush_server (c->server, timeout) < 0*/ do_rpc_flush(timeout)) {
    value.type = MEMCACHE_ERROR;
    value.val.strval.data = global_error;
    value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
    return value;
  }
  if (do_rpc_get_and_parse(qid, timeout - precise_now) < 0) {
    value.type = MEMCACHE_ERROR;
    value.val.strval.data = const_cast<char *>("timeout");
    value.val.strval.len = strlen(value.val.strval.data);
    return value;
  }
  return do_memcache_parse_value();
}

struct memcache_value do_memcache_get(struct rpc_connection *c, const char *key, int key_len, double timeout, int wait, int fake) {
  long long qid = do_memcache_send_get(c, key, key_len, timeout, wait, fake);
  if (wait) {
    return do_memcache_act(c, qid, 1, timeout);
  } else {
    static struct memcache_value value;
    if (qid < 0) {
      value.type = MEMCACHE_ERROR;
      value.val.strval.data = global_error;
      value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
    } else {
      if (do_rpc_flush(timeout)) {
        value.type = MEMCACHE_ERROR;
        value.val.strval.data = global_error;
        value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
        return value;
      }
      value.type = MEMCACHE_TRUE;
    }
    return value;
  }
}

struct memcache_value do_memcache_store(struct rpc_connection *c, const char *key, int key_len, const char *value, int value_len, int flags, int delay, int op, double timeout, int wait) {
  assert(vk::any_of_equal(static_cast<unsigned int>(op), MEMCACHE_SET, MEMCACHE_REPLACE, MEMCACHE_ADD, TL_ENGINE_MC_SET_QUERY, TL_ENGINE_MC_ADD_QUERY, TL_ENGINE_MC_REPLACE_QUERY));
  long long qid = do_memcache_send_store(c, key, key_len, value, value_len, flags, delay, op, timeout, wait);
  if (wait) {
    return do_memcache_act(c, qid, 1, timeout);
  } else {
    static struct memcache_value value;
    if (qid < 0) {
      value.type = MEMCACHE_ERROR;
      value.val.strval.data = global_error;
      value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
    } else {
      if (do_rpc_flush(timeout)) {
        value.type = MEMCACHE_ERROR;
        value.val.strval.data = global_error;
        value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
        return value;
      }
      value.type = MEMCACHE_TRUE;
    }
    return value;
  }
}

struct memcache_value do_memcache_incr(struct rpc_connection *c, const char *key, int key_len, long long val, int op, double timeout, int wait) {
  assert (vk::any_of_equal(static_cast<unsigned int>(op), MEMCACHE_INCR, MEMCACHE_DECR, TL_ENGINE_MC_INCR_QUERY, TL_ENGINE_MC_DECR_QUERY));
  long long qid = do_memcache_send_incr(c, key, key_len, val, op, timeout, wait);
  if (wait) {
    return do_memcache_act(c, qid, 1, timeout);
  } else {
    static struct memcache_value value;
    if (qid < 0) {
      value.type = MEMCACHE_ERROR;
      value.val.strval.data = global_error;
      value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
    } else {
      if (do_rpc_flush(timeout)) {
        value.type = MEMCACHE_ERROR;
        value.val.strval.data = global_error;
        value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
        return value;
      }
      value.type = MEMCACHE_VALUE_LONG;
      value.val.lval = 0;
      value.flags = 0;
    }
    return value;
  }
}

struct memcache_value do_memcache_delete(struct rpc_connection *c, const char *key, int key_len, double timeout, int wait, int fake) {
  long long qid = do_memcache_send_delete(c, key, key_len, timeout, wait, fake);
  if (wait) {
    return do_memcache_act(c, qid, 1, timeout);
  } else {
    static struct memcache_value value;
    if (qid < 0) {
      value.type = MEMCACHE_ERROR;
      value.val.strval.data = global_error;
      value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
    } else {
      if (do_rpc_flush(timeout)) {
        value.type = MEMCACHE_ERROR;
        value.val.strval.data = global_error;
        value.val.strval.len = value.val.strval.data ? strlen(value.val.strval.data) : 0;
        return value;
      }
      value.type = MEMCACHE_TRUE;
    }
    return value;
  }
}

int get_fd(zval *obj) {
  VK_ZVAL_API_P r = vk_zend_hash_find(Z_OBJPROP_P(obj), "fd", VK_STR_API_LEN(strlen("fd")));
  if (r) {
    return Z_LVAL_P (VK_ZVAL_API_TO_ZVALP(r));
  } else {
    return -1;
  }
}

int get_fake(zval *obj) {
  VK_ZVAL_API_P r = vk_zend_hash_find(Z_OBJPROP_P(obj), "fake", VK_STR_API_LEN(strlen("fake")));
  if (r) {
    return VK_ZVAL_IS_TRUE(VK_ZVAL_API_TO_ZVALP(r));
  } else {
    return 0;
  }
}


static int mmc_postprocess_value(zval **return_value, char *value, int value_len TSRMLS_DC) {
  const char *value_tmp = value;
  php_unserialize_data_t var_hash;
  PHP_VAR_UNSERIALIZE_INIT(var_hash);

  if (!php_var_unserialize(Z_PP_TO_ZAPI_P(return_value), (const unsigned char **)&value_tmp, (const unsigned char *)(value_tmp + value_len), &var_hash TSRMLS_CC)) {
    ZVAL_FALSE(*return_value);
    PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
    php_error_docref(NULL TSRMLS_CC, E_NOTICE, "unable to unserialize data");
    return 0;
  }

  PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
  return 1;
}

void php_rpc_fetch_memcache_value(INTERNAL_FUNCTION_PARAMETERS) {
  struct memcache_value value = do_fetch_memcache_value();
  switch (value.type) {
    case MEMCACHE_VALUE_LONG: RETURN_LONG (value.val.lval);
      break;
    case MEMCACHE_VALUE_STRING:
      if (value.flags & 2) {
        char *res = 0;
        unsigned long res_len = 0;
        if (!mmc_uncompress(&res, &res_len, value.val.strval.data, value.val.strval.len)) {
          RETURN_FALSE;
        }

        if (value.flags & 1) {
          mmc_postprocess_value(&return_value, res, res_len TSRMLS_DC);
          efree (res);
        } else {
          VK_RETURN_STRINGL_NOD (res, res_len);
        }
        break;
      } else {
        if (value.flags & 1) {
          mmc_postprocess_value(&return_value, value.val.strval.data, value.val.strval.len);
        } else {
          ADD_RMALLOC (value.val.strval.len + 1);
          VK_RETURN_STRINGL_DUP (value.val.strval.data, value.val.strval.len);
        }
        break;
      }
    case MEMCACHE_ERROR:
    case MEMCACHE_FALSE:
    case MEMCACHE_VALUE_NOT_FOUND:
    default: RETURN_FALSE;
  }
}

void rpc_memcache_multiget(INTERNAL_FUNCTION_PARAMETERS);

void rpc_memcache_get(INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[5];
  if (argc < 1) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (zend_get_parameters_array_ex (argc > 2 ? 2 : argc, z) == FAILURE) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (VK_ZVAL_API_ARRAY_EL_TYPE (z[0]) == IS_ARRAY) {
    END_TIMER (parse);
    rpc_memcache_multiget(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    return;
  }

  zval *obj = getThis ();
  if (!obj) {
    RETURN_FALSE;
  }
  int fd = get_fd(obj);
  if (fd < 0) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  //struct rpc_server *server = rpc_server_get (fd);
  struct rpc_connection *c = rpc_connection_get(fd);
//  if (!c || !c->server || c->server->status != rpc_status_connected) {
  if (!c || !c->servers) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  int l;
  char *key = parse_zend_string(VK_ZVAL_ARRAY_TO_API_P(z[0]), &l);
  int hat = 0;
  if (*key == '^') {
    key++;
    l--;
    hat = 1;
  }

  double timeout = c->default_query_timeout;
  if (argc > 1) {
    timeout = parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[1]));
  }
  update_precise_now();
  timeout += precise_now;
  END_TIMER (parse);

  struct memcache_value value = do_memcache_get(c, key, l, timeout, !hat, get_fake(obj));


  switch (value.type) {
    case MEMCACHE_TRUE: RETURN_TRUE;
      break;
    case MEMCACHE_VALUE_LONG: RETURN_LONG (value.val.lval);
      break;
    case MEMCACHE_VALUE_STRING:
      if (value.flags & 2) {
        char *res = 0;
        unsigned long res_len = 0;
        if (!mmc_uncompress(&res, &res_len, value.val.strval.data, value.val.strval.len)) {
          RETURN_FALSE;
        }

        if (value.flags & 1) {
          mmc_postprocess_value(&return_value, res, res_len TSRMLS_DC);
          efree (res);
        } else {
          VK_RETURN_STRINGL_NOD(res, res_len);
        }
        break;
      } else {
        if (value.flags & 1) {
          mmc_postprocess_value(&return_value, value.val.strval.data, value.val.strval.len);
        } else {
          ADD_RMALLOC (value.val.strval.len + 1);
          VK_RETURN_STRINGL_DUP (value.val.strval.data, value.val.strval.len);
        }
        break;
      }
    case MEMCACHE_ERROR:
    case MEMCACHE_FALSE:
    case MEMCACHE_VALUE_NOT_FOUND:
    default: RETURN_FALSE;
  }
}

void rpc_memcache_multiget(INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[5];
  if (argc < 1) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (zend_get_parameters_array_ex (argc > 2 ? 2 : argc, z) == FAILURE) {
    END_TIMER (parse);
    RETURN_FALSE;
  }

  zval *obj = getThis ();
  if (!obj) {
    RETURN_FALSE;
  }
  int fd = get_fd(obj);
  if (fd < 0) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  struct rpc_connection *c = rpc_connection_get(fd);
//  if (!c || !c->server || c->server->status != rpc_status_connected) {
  if (!c || !c->servers) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (VK_ZVAL_API_ARRAY_EL_TYPE(z[0]) != IS_ARRAY) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  double timeout = c->default_query_timeout;
  if (argc > 1) {
    timeout = parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[1]));
  }
  update_precise_now();
  timeout += precise_now;

  array_init (return_value);

  VK_ZVAL_API_P zkey;
  int num = zend_hash_num_elements (VK_Z_API_ARRVAL((VK_ZVAL_ARRAY_TO_API_P(z[0]))));
  long long *qids = static_cast<long long *>(malloc(sizeof(long long) * num));
  VK_ZVAL_API_P *keys = reinterpret_cast<VK_ZVAL_API_P *>(malloc(sizeof(void *) * num));
  int cc = 0;
  int cc1 = 0;

  int l = 0;
  char *s = NULL;

  VK_ZEND_HASH_FOREACH_VAL(VK_Z_API_ARRVAL(VK_ZVAL_ARRAY_TO_API_P(z[0])), zkey) {
    s = parse_zend_string(zkey, &l);
//    int hat = 0;
    if (*s == '^') {
      s++;
      l--;
//      hat = 1;
    }

    long long qid = do_memcache_send_get(c, s, l, timeout, 1, get_fake(obj)); // TODO: make use of hat
    if (qid > 0) {
      keys[cc] = zkey;
      qids[cc++] = qid;
    } else {
      ++cc1;
      /*free (qids);
      free (keys);
      zval_dtor(return_value);
      END_TIMER (parse);
      RETURN_FALSE;
      return;*/
    }
  } VK_ZEND_HASH_FOREACH_END();

  if (cc1 > 0) {
    fprintf(stderr, "vkext: Skipping RMC queries: %d of %d skipped (last key: %.*s)\n", cc1, cc1 + cc,
      l, ((l > 0) ? s : ""));
  }
  END_TIMER (parse);
//  if (do_rpc_flush_server (c->server, timeout) < 0) {
  if (do_rpc_flush(timeout)) {
    free(qids);
    free(keys);
    zval_dtor(return_value);
    RETURN_FALSE;
    return;
  }
  if (!cc) {
    free(qids);
    free(keys);
    return;
  }
  long long QN = do_rpc_queue_create(cc, qids);
  if (!QN) {
    free(qids);
    free(keys);
    zval_dtor(return_value);
    RETURN_FALSE;
    return;
  }
  struct rpc_queue *Q = rpc_queue_get(QN);
  assert (Q);
  int i;
  for (i = 0; i < cc; i++) {
    long long qid = do_rpc_queue_next(Q, timeout);
    if (qid <= 0) {
      break;
    }
    /*long long qid = do_rpc_get_any_qid (timeout);
    if (qid < 0) { 
      break;
    }
    if (qid < qids[0]) { continue; }*/
    assert (qid >= qids[0]);
    int k = qid - qids[0];
    if (k >= cc || qids[k] != qid) {
      int l = 0;
      int r = cc;
      while (r - l > 1) {
        int x = (r + l) >> 1;
        if (qids[x] <= qid) {
          l = x;
        } else {
          r = x;
        }
      }
      assert (qids[l] == qid);
      k = l;
    }

    assert (k < cc);
    if (do_rpc_get_and_parse(qid, timeout - precise_now) < 0) {
      continue;
    }
    struct memcache_value value = do_memcache_parse_value();
    char *data = value.val.strval.data;
    VK_LEN_T data_len = value.val.strval.len;
    char *res = 0;
    int ok = 1;

    int l;
    char *s = parse_zend_string(keys[k], &l);
    if (*s == '^') {
      s++;
      l--;
    }
    if ((value.flags & 2) && value.type == MEMCACHE_VALUE_STRING) {
      unsigned long res_len = 0;
      if (!mmc_uncompress(&res, &res_len, data, data_len)) {
        //TODO:check
        add_assoc_bool_ex(return_value, s, VK_STR_API_LEN(l), 0);
        ok = 0;
      } else {
        data = res;
        data_len = res_len;
      }
      free(value.val.strval.data);
    }
    if (ok) {
      if (value.flags & 1) {
        zval *zp;
        switch (value.type) {
          case MEMCACHE_VALUE_STRING:
            assert (k < cc);
            VK_ALLOC_INIT_ZVAL(zp);
            mmc_postprocess_value(&zp, data, data_len TSRMLS_DC);
            if (!res) {
              free(data);
            }
            vk_add_assoc_zval_ex_nod (return_value, s, VK_STR_API_LEN(l), zp);
            break;
          case MEMCACHE_VALUE_LONG:
            add_assoc_long_ex(return_value, s, VK_STR_API_LEN(l), value.val.lval);
            break;
            //case MEMCACHE_VALUE_NOT_FOUND:
            //  add_assoc_bool_ex (return_value, Z_STRVAL_PP (keys[k]), Z_STRLEN_PP (keys[k]) + 1, 0);
            //  break;
        }
      } else {
        switch (value.type) {
          case MEMCACHE_VALUE_STRING:
            if (!res) {
              vk_add_assoc_stringl_dup (return_value, s, VK_STR_API_LEN(l), data, data_len);
            } else {
              vk_add_assoc_stringl_nod (return_value, s, VK_STR_API_LEN(l), data, data_len);
              res = 0;
            }
            break;
          case MEMCACHE_VALUE_LONG:
            add_assoc_long_ex(return_value, s, VK_STR_API_LEN(l), value.val.lval);
            break;
            //case MEMCACHE_VALUE_NOT_FOUND:
            //  add_assoc_bool_ex (return_value, Z_STRVAL_PP (keys[k]), Z_STRLEN_PP (keys[k]) + 1, 0);
            //  break;
        }
      }
      if (res) {
        efree (res);
      }
    }
  }
  for (i = 0; i < cc; i++) {
    do_rpc_get_and_parse(qids[i], 0);
  }
  free(qids);
  free(keys);
  do_rpc_queue_free(QN);
}

void rpc_memcache_store(int op, INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[6];
  if (argc < 2) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (zend_get_parameters_array_ex (argc > 5 ? 5 : argc, z) == FAILURE) {
    END_TIMER (parse);
    RETURN_FALSE;
  }

  zval *obj = getThis ();
  if (!obj) {
    RETURN_FALSE;
  }
  int fd = get_fd(obj);
  if (fd < 0) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  struct rpc_connection *c = rpc_connection_get(fd);
//  if (!c || !c->server || c->server->status != rpc_status_connected) {
  if (!c || !c->servers) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  int key_len;
  char *key = parse_zend_string(VK_ZVAL_ARRAY_TO_API_P(z[0]), &key_len);
  int hat = 0;
  if (*key == '^') {
    key++;
    key_len--;
    hat = 1;
  }
//  int value_len;
//  char *value = parse_zend_string (z[1], &value_len);
  int flags = 0;
  if (argc >= 3) {
    flags = parse_zend_long(VK_ZVAL_ARRAY_TO_API_P(z[2]));
  }
  int delay = 0;
  if (argc >= 4) {
    delay = parse_zend_long(VK_ZVAL_ARRAY_TO_API_P(z[3]));
  }
  double timeout = c->default_query_timeout;
  if (argc >= 5) {
    timeout = parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[4]));
  }
  update_precise_now();
  timeout += precise_now;
  END_TIMER (parse);

  if (op == 0) {
    op = get_fake(obj) ? TL_ENGINE_MC_SET_QUERY : MEMCACHE_SET;
  } else if (op == 1) {
    op = get_fake(obj) ? TL_ENGINE_MC_ADD_QUERY : MEMCACHE_ADD;
  } else {
    op = get_fake(obj) ? TL_ENGINE_MC_REPLACE_QUERY : MEMCACHE_REPLACE;
  }

  struct memcache_value ans;
  zval *value = VK_ZVAL_API_TO_ZVALP(VK_ZVAL_ARRAY_TO_API_P(z[1]));
  switch (Z_TYPE_P(value)) {
    case IS_STRING:
      ans = do_memcache_store(c, key, key_len, Z_STRVAL_P (value), Z_STRLEN_P (value), flags, delay, op, timeout, !hat);
      break;

    case IS_LONG:
    case IS_DOUBLE:
    case VK_IS_BOOL_T_CASE: {
      zval value_copy;

      /* FIXME: we should be using 'Z' instead of this, but unfortunately it's PHP5-only */
      value_copy = *value;
      zval_copy_ctor(&value_copy);
      convert_to_string(&value_copy);

      ans = do_memcache_store(c, key, key_len, Z_STRVAL (value_copy), Z_STRLEN (value_copy), flags, delay, op, timeout, !hat);

      zval_dtor(&value_copy);
      break;
    }

    default: {
      zval value_copy, *value_copy_ptr;

      /* FIXME: we should be using 'Z' instead of this, but unfortunately it's PHP5-only */
      value_copy = *value;
      zval_copy_ctor(&value_copy);
      value_copy_ptr = &value_copy;
      php_serialize_data_t value_hash;
      smart_str buf = {0};

      PHP_VAR_SERIALIZE_INIT(value_hash);
      php_var_serialize(&buf, ZP_TO_API_P(value_copy_ptr), &value_hash TSRMLS_CC);
      PHP_VAR_SERIALIZE_DESTROY(value_hash);

      if (!SMART_STRDATA(buf)) {
        /* something went really wrong */
        zval_dtor(&value_copy);
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to serialize value");
        RETURN_FALSE;
      }

      flags |= 1;
      zval_dtor(&value_copy);

      ans = do_memcache_store(c, key, key_len, SMART_STRVAL(buf), SMART_STRLEN(buf), flags, delay, op, timeout, !hat);
    }
  }

  //struct memcache_value ans = do_memcache_store (c, key, key_len, value, value_len, flags, delay, op, timeout);
  switch (ans.type) {
    case MEMCACHE_TRUE: RETURN_TRUE;
      break;
    case MEMCACHE_ERROR:
    case MEMCACHE_VALUE_LONG:
    case MEMCACHE_VALUE_STRING:
    case MEMCACHE_FALSE:
    case MEMCACHE_VALUE_NOT_FOUND:
    default: RETURN_FALSE;
  }

}


void rpc_memcache_incr(int op, INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[5];
  if (argc < 1) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (zend_get_parameters_array_ex (argc > 3 ? 3 : argc, z) == FAILURE) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  zval *obj = getThis ();
  if (!obj) {
    RETURN_FALSE;
  }
  int fd = get_fd(obj);
  if (fd < 0) {
    END_TIMER (parse);
    RETURN_FALSE;
  }

  struct rpc_connection *c = rpc_connection_get(fd);
  if (!c || !c->servers) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  int l;
  char *key = parse_zend_string(VK_ZVAL_ARRAY_TO_API_P(z[0]), &l);
  int hat = 0;
  if (*key == '^') {
    key++;
    l--;
    hat = 1;
  }
  long long val;
  if (argc > 1) {
    val = parse_zend_long(VK_ZVAL_ARRAY_TO_API_P(z[1]));
  } else {
    val = 1;
  }

  double timeout = c->default_query_timeout;
  if (argc > 2) {
    timeout = parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[2]));
  }
  update_precise_now();
  timeout += precise_now;
  END_TIMER (parse);
  if (op == 0) {
    op = get_fake(obj) ? TL_ENGINE_MC_INCR_QUERY : MEMCACHE_INCR;
  } else {
    op = get_fake(obj) ? TL_ENGINE_MC_DECR_QUERY : MEMCACHE_DECR;
  }
  struct memcache_value value = do_memcache_incr(c, key, l, val, op, timeout, !hat);
  switch (value.type) {
    case MEMCACHE_VALUE_LONG: RETURN_LONG (value.val.lval);
      break;
    case MEMCACHE_VALUE_STRING:
      ADD_RMALLOC (value.val.strval.len + 1);
      VK_RETVAL_STRINGL_DUP (value.val.strval.data, value.val.strval.len);
      break;
    case MEMCACHE_ERROR:
    case MEMCACHE_TRUE:
    case MEMCACHE_FALSE:
    case MEMCACHE_VALUE_NOT_FOUND:
    default: RETURN_FALSE;
  }
}

void rpc_memcache_delete(INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  zval *obj = getThis ();
  if (!obj) {
    RETURN_FALSE;
  }
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[5];
  if (argc < 1) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (zend_get_parameters_array_ex (argc > 2 ? 2 : argc, z) == FAILURE) {
    END_TIMER (parse);
    RETURN_FALSE;
  }

  int fd = get_fd(obj);
  if (fd < 0) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  struct rpc_connection *c = rpc_connection_get(fd);
  //if (!c || !c->server || c->server->status != rpc_status_connected) {
  if (!c || !c->servers) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  int l;
  char *key = parse_zend_string(VK_ZVAL_ARRAY_TO_API_P(z[0]), &l);
  int hat = 0;
  if (*key == '^') {
    key++;
    l--;
    hat = 1;
  }

  double timeout = c->default_query_timeout;
  if (argc > 1) {
    timeout = parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[1]));
  }
  update_precise_now();
  timeout += precise_now;
  END_TIMER (parse);
  struct memcache_value value = do_memcache_delete(c, key, l, timeout, !hat, get_fake(obj));
  switch (value.type) {
    case MEMCACHE_TRUE: RETURN_TRUE;
      break;
    case MEMCACHE_ERROR:
    case MEMCACHE_VALUE_STRING:
    case MEMCACHE_VALUE_LONG:
    case MEMCACHE_FALSE:
    case MEMCACHE_VALUE_NOT_FOUND:
    default: RETURN_FALSE;
  }

}

void rpc_memcache_connect(INTERNAL_FUNCTION_PARAMETERS) {
  zval *obj = getThis ();
  if (!obj) {
    RETURN_FALSE;
  }
  php_new_rpc_connection(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  if (Z_TYPE_P (return_value) == IS_LONG) {
    long long t = Z_LVAL_P (return_value);
    add_property_long (obj, "fd", t);
    RETURN_TRUE;
  }
  RETURN_FALSE;
}
