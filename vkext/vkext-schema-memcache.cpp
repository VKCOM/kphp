// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "vkext/vkext-schema-memcache.h"

#include <errno.h>
#include <cinttypes>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/algorithms/find.h"
#include "common/c-tree.h"
#include "common/crc32.h"
#include "common/rpc-error-codes.h"
#include "common/tl/compiler/tl-tl.h"
#include "common/tl/constants/common.h"

#include "vkext/vkext-errors.h"
#include "vkext/vkext-rpc-include.h"
#include "vkext/vkext-rpc.h"
#include "vkext/vkext-tl-parse.h"
#include "vkext/vkext.h"

#ifndef Z_ADDREF_P
#define Z_ADDREF_P(ptr) (ptr)->refcount++;
#define Z_ADDREF_PP(ptr) Z_ADDREF_P(*(ptr))
#endif

#define MEMCACHE_ERROR 0x7ae432f5
#define MEMCACHE_VALUE_NOT_FOUND 0x32c42422
#define MEMCACHE_VALUE_LONG 0x9729c42
#define MEMCACHE_VALUE_STRING 0xa6bebb1a
#define MEMCACHE_FALSE 0xbc799737
#define MEMCACHE_TRUE 0x997275b5
#define MEMCACHE_SET 0xeeeb54c4
#define MEMCACHE_ADD 0xa358f31c
#define MEMCACHE_REPLACE 0x2ecdfaa2
#define MEMCACHE_INCR 0x80e6c950
#define MEMCACHE_DECR 0x6467e0d9
#define MEMCACHE_DELETE 0xab505c0a
#define MEMCACHE_GET 0xd33b13ae

//#define VLOG

#ifdef VLOG
#define Z fprintf (stderr, "%d\n", __LINE__);
#else
#define Z
#endif
int total_ref_cnt;
int persistent_tree_nodes;
int dynamic_tree_nodes;
extern int verbosity;
int total_tl_working;
int total_tree_nodes_existed;

/* HASH Tables {{{ */

static unsigned string_hash(const char *s) {
  unsigned h = 0;
  while (*s) {
    h = h * 17 + *s;
    s++;
  }
  return h;
}

#define tl_type_id_cmp(a, b) (strcmp ((a)->id, (b)->id))
#define tl_type_id_hash(a) (string_hash ((a)->id))
DEFINE_HASH_STDNOZ_ALLOC(tl_type_id, struct tl_type *, tl_type_id_cmp, tl_type_id_hash);

/*struct hash_elem_tl_type_id *tl_type_id_arr[(1 << 12)];
struct hash_table_tl_type_id tl_type_id_hash_table = {
  .size = (1 << 12),
  .E = tl_type_id_arr,
  .mask = (1 << 12) - 1
};*/

#define tl_type_name_cmp(a, b) ((a)->name - (b)->name)
#define tl_type_name_hash(a) ((a)->name)
DEFINE_HASH_STDNOZ_ALLOC(tl_type_name, struct tl_type *, tl_type_name_cmp, tl_type_name_hash);

/*struct hash_elem_tl_type_name *tl_type_name_arr[(1 << 12)];
struct hash_table_tl_type_name tl_type_name_hash_table = {
  .size = (1 << 12),
  .E = tl_type_name_arr,
  .mask = (1 << 12) - 1
};*/

DEFINE_HASH_STDNOZ_ALLOC(tl_fun_id, struct tl_combinator *, tl_type_id_cmp, tl_type_id_hash);

/*struct hash_elem_tl_fun_id *tl_fun_id_arr[(1 << 12)];
struct hash_table_tl_fun_id tl_fun_id_hash_table = {
  .size = (1 << 12),
  .E = tl_fun_id_arr,
  .mask = (1 << 12) - 1
};*/

DEFINE_HASH_STDNOZ_ALLOC(tl_fun_name, struct tl_combinator *, tl_type_name_cmp, tl_type_name_hash);

/*struct hash_elem_tl_fun_name *tl_fun_name_arr[(1 << 12)];
struct hash_table_tl_fun_name tl_fun_name_hash_table = {
  .size = (1 << 12),
  .E = tl_fun_name_arr,
  .mask = (1 << 12) - 1
};*/

struct tl_config {
  int fn, cn, tn, pos;
  struct tl_type **tps;
  struct tl_combinator **fns;
  struct hash_table_tl_fun_name *ht_fname;
  struct hash_table_tl_fun_id *ht_fid;
  struct hash_table_tl_type_name *ht_tname;
  struct hash_table_tl_type_id *ht_tid;
  int working_queries;
};

struct tl_config *cur_config;
#define CONFIG_LIST_SIZE 10
struct tl_config *config_list[CONFIG_LIST_SIZE];
int config_list_pos;

int typed_mode = 0;
const char *tl_current_function_name;

static int is_tl_type_flat(const struct tl_type *t, int *arg_idx) {
  int _arg_idx = -1;
  if (!arg_idx) {
    arg_idx = &_arg_idx;
  }
  int not_optional_args_cnt = 0;
  if (t->constructors_num != 1) {
    return false;
  }
  const struct tl_combinator *c = t->constructors[0];
  for (int i = 0; i < c->args_num; i++) {
    if (!(c->args[i]->flags & FLAG_OPT_VAR)) {
      *arg_idx = i;
      not_optional_args_cnt++;
    }
  }
  if (not_optional_args_cnt != 1) {
    return false;
  }
  if (strcasecmp(t->id, c->id)) {
    return false;
  }
  if (c->args[*arg_idx]->flags & FLAG_OPT_FIELD) {
    return false;
  }
  return true;
}

struct arg *get_first_explicit_arg(struct tl_combinator *c) {
  for (int i = 0; i < c->args_num; ++i) {
    struct arg *cur_arg = c->args[i];
    if (!(cur_arg->flags & FLAG_OPT_VAR)) {
      return cur_arg;
    }
  }
  return NULL;
}

static void tl_tree_debug_print(struct tl_tree *tree, int shift) __attribute__((unused));
static void tl_tree_debug_print(struct tl_tree *tree, int shift) {
  for (int i = 0; i < shift; ++i) {
    fprintf(stderr, "  ");
  }
  switch (TYPE(tree)) {
    case NODE_TYPE_TYPE: {
      struct tl_tree_type *as_type = (struct tl_tree_type *)tree;
      fprintf(stderr, "tl type: %s\n", as_type->type->id);
      for (int i = 0; i < as_type->children_num; ++i) {
        tl_tree_debug_print(as_type->children[i], shift + 1);
      }
      break;
    }
    case NODE_TYPE_NAT_CONST: {
      struct tl_tree_nat_const *as_nat_const = (struct tl_tree_nat_const *)tree;
      fprintf(stderr, "natural constant: %lld\n", as_nat_const->value);
      break;
    }
    case NODE_TYPE_VAR_TYPE: {
      struct tl_tree_var_type *as_type_var = (struct tl_tree_var_type *)tree;
      fprintf(stderr, "type variable: points to argument with var_num = %d\n", as_type_var->var_num);
      break;
    }
    case NODE_TYPE_VAR_NUM: {
      struct tl_tree_var_num *as_num_var = (struct tl_tree_var_num *)tree;
      fprintf(stderr, "numerical variable: points to argument with var_num = %d\n", as_num_var->var_num);
      break;
    }
    case NODE_TYPE_ARRAY: {
      struct tl_tree_array *as_type_array = (struct tl_tree_array *)tree;
      fprintf(stderr, "tl builtin array: item length = %d; array length:\n", as_type_array->args_num);
      tl_tree_debug_print(as_type_array->multiplicity, shift + 1);
      for (int i = 0; i < as_type_array->args_num; ++i) {
        tl_tree_debug_print(as_type_array->args[i]->type, shift + 1);
      }
      break;
    }
    default:
      assert(false);
  }
}

static const char *reqResult_underscore_class_name = "VK\\TL\\_common\\Types\\rpcResponseOk";
static const char *reqResult_header_class_name = "VK\\TL\\_common\\Types\\rpcResponseHeader";
static const char *reqResult_error_class_name = "VK\\TL\\_common\\Types\\rpcResponseError";
static const char *rpcFunctionReturnResult_class_name = "VK\\TL\\RpcFunctionReturnResult";
static const char *true_type_class_name = "VK\\TL\\_common\\Types\\true";
static const char *vk_tl_prefix = "VK\\TL\\";
static const char *php_common_namespace = "_common";

#define PHP_CLASS_NAME_BUFFER_LENGTH 1000 // for names like "VK\TL\_common\Types\VectorTotal__VectorTotal__VectorTotal__int"
#define PHP_PREFIX_BUFFER_LENGTH 100      // for "VK\TL\_common\Types\" part

void check_buffer_overflow(int len) {
  if (len > PHP_CLASS_NAME_BUFFER_LENGTH) {
    php_error_docref(NULL, E_ERROR,
                     "Internal class name buffer overflow (rpc typed mode)");
  }
}

#define MAX_DEPTH 10000

struct tl_type *tl_type_get_by_id(const char *s) {
  assert (cur_config);
  struct tl_type t;
  t.id = (char *)s;
  //struct hash_elem_tl_type_id *h = hash_lookup_tl_type_id (&tl_type_id_hash_table, &t);
  struct hash_elem_tl_type_id *h = hash_lookup_tl_type_id(cur_config->ht_tid, &t);
  return h ? h->x : 0;
}

struct tl_combinator *tl_fun_get_by_id(const char *s) {
  assert (cur_config);
  struct tl_combinator c;
  c.id = (char *)s;
  //struct hash_elem_tl_fun_id *h = hash_lookup_tl_fun_id (&tl_fun_id_hash_table, &c);
  struct hash_elem_tl_fun_id *h = hash_lookup_tl_fun_id(cur_config->ht_fid, &c);
  return h ? h->x : 0;
}

struct tl_combinator *tl_fun_get_by_name(int name) {
  assert (cur_config);
  struct tl_combinator c;
  c.name = name;
  //struct hash_elem_tl_fun_name *h = hash_lookup_tl_fun_name (&tl_fun_name_hash_table, &c);
  struct hash_elem_tl_fun_name *h = hash_lookup_tl_fun_name(cur_config->ht_fname, &c);
  return h ? h->x : 0;
}

struct tl_type *tl_type_get_by_name(int name) {
  assert (cur_config);
  struct tl_type t;
  t.name = name;
  //struct hash_elem_tl_type_name *h = hash_lookup_tl_type_name (&tl_type_name_hash_table, &t);
  struct hash_elem_tl_type_name *h = hash_lookup_tl_type_name(cur_config->ht_tname, &t);
  return h ? h->x : 0;
}

void tl_type_insert_name(struct tl_type *t) {
  assert (cur_config);
  if (t->name) {
    assert (!tl_type_get_by_name(t->name));
    //hash_insert_tl_type_name (&tl_type_name_hash_table, t);
    hash_insert_tl_type_name(cur_config->ht_tname, t);
  } else {
    if (!tl_type_get_by_name(t->name)) {
      //hash_insert_tl_type_name (&tl_type_name_hash_table, t);
      hash_insert_tl_type_name(cur_config->ht_tname, t);
    }
  }
}

void tl_type_insert_id(struct tl_type *t) {
  assert (cur_config);
  assert (!tl_type_get_by_id(t->id));
  //hash_insert_tl_type_id (&tl_type_id_hash_table, t);
  hash_insert_tl_type_id(cur_config->ht_tid, t);
}

void tl_fun_insert_id(struct tl_combinator *t) {
  assert (cur_config);
  assert (!tl_fun_get_by_id(t->id));
  //hash_insert_tl_fun_id (&tl_fun_id_hash_table, t);
  hash_insert_tl_fun_id(cur_config->ht_fid, t);
}

void tl_fun_insert_name(struct tl_combinator *t) {
  assert (cur_config);
  assert (!tl_fun_get_by_name(t->name));
  //hash_insert_tl_fun_name (&tl_fun_name_hash_table, t);
  hash_insert_tl_fun_name(cur_config->ht_fname, t);
}

/* }}} */

/**
 * "VK\TL\(namespace)\Function\(namespace)_name" -> namespace
 */
static void get_php_namespace_by_php_class_name(char *dst, const char *php_tl_class_name) {
  const char *short_php_tl_class_name = php_tl_class_name + strlen(vk_tl_prefix);
  const char *slash_ptr = strchr(short_php_tl_class_name, '\\');
  assert(slash_ptr != NULL);
  int res_str_len = slash_ptr - short_php_tl_class_name;
  strncpy(dst, short_php_tl_class_name, res_str_len);
  dst[res_str_len] = 0;
  check_buffer_overflow(res_str_len + 1);
}

/**
 * "(namespace).name" -> namespace or
 * "name" -> "_common"
 */
static void get_php_namespace_by_tl_name(char *dst, const char *tl_name) {
  const char *dot_ptr = strchr(tl_name, '.');
  if (dot_ptr == NULL) {
    strcpy(dst, php_common_namespace);
    return;
  }
  int dot_pos = dot_ptr - tl_name;
  strncpy(dst, tl_name, dot_pos);
  dst[dot_pos] = 0;
}

enum php_prefix_t {
  FUNCTIONS,
  TYPES
};

static void get_php_prefix(char *dst, const char *tl_name, enum php_prefix_t php_prefix_type) {
  static char php_namespace[50];
  get_php_namespace_by_tl_name(php_namespace, tl_name);
  strcpy(dst, vk_tl_prefix);
  strcat(dst, php_namespace);
  if (php_prefix_type == FUNCTIONS) {
    strcat(dst, "\\Functions\\");
  } else {
    strcat(dst, "\\Types\\");
  }
}

static int make_tl_class_name(char *dst, const char *prefix, const char *tl_name, const char *suffix, char new_delim) {
  strcpy(dst, prefix);
  int prefix_len = strlen(prefix);
  int tl_name_len = strlen(tl_name);
  for (int i = 0; i < tl_name_len; ++i) {
    char cur_c = tl_name[i];
    dst[i + prefix_len] = (cur_c == '.' ? new_delim : cur_c);
  }
  strcpy(dst + prefix_len + tl_name_len, suffix);
  return prefix_len + tl_name_len + strlen(suffix);
}

/**
 * The same as tl_gen::is_tl_type_a_php_array(...)
 */
int is_php_array(struct tl_type *t) {
  if (t->name == TL_VECTOR || t->name == TL_TUPLE || t->name == TL_DICTIONARY ||
      t->name == TL_INT_KEY_DICTIONARY || t->name == TL_LONG_KEY_DICTIONARY) {
    return 1;
  } else {
    return 0;
  }
}

int is_php_builtin(struct tl_type *t) {
  if (t->name == TL_INT || t->name == TL_DOUBLE || t->name == TL_FLOAT || t->name == TL_LONG || t->name == TL_STRING ||
      !strcmp(t->id, "Bool") || !strcmp(t->id, "Maybe") || !strcmp(t->id, "#")) {
    return 1;
  } else {
    return 0;
  }
}

bool is_arg_named_fields_mask_bit(struct arg *arg) {
  if (!(arg->flags & FLAG_OPT_FIELD) || TYPE(arg->type) != NODE_TYPE_TYPE) {
    return false;
  }
  struct tl_tree_type *as_type = (struct tl_tree_type *)arg->type;
  return as_type->type->name == TL_TRUE && as_type->self.flags & FLAG_BARE;
}

/**
 * Generates php tl class name by tl type tree recursively
 */
int get_full_tree_type_name(char *dst, struct tl_tree_type *tree, const char *constructor_name) {
  char *start_dst = dst;
  struct tl_type *cur_type = tree->type;
  // {{
  // At this part we make the main part of name, e.g. "vectorTotal", "array", "maybe"
  if (constructor_name) {
    // construct_name != NULL only in the first call
    dst += make_tl_class_name(dst, "", constructor_name, "", '_');
  } else {
    // in every recursive call it's NULL
    // tree is child here!
    if (tree->self.flags & FLAG_FORWARDED) {
      // if it's got from forwarded function (via !), parent type is parametrized by "RpcFunctionReturnResult"
      // instead of whole calculated subtree
      dst += make_tl_class_name(dst, "", "RpcFunctionReturnResult", "", '\0');
      return dst - start_dst;
    }
    if (is_php_array(cur_type)) {
      // if its type is php array, it's called uniformly
      dst += make_tl_class_name(dst, "", "array", "", '\0');
    } else if (!strcmp(cur_type->id, "Maybe")) {
      // in case of (Maybe T) in php it can be:
      // 1) Optional<T::PhpType> -- Maybe_T::PhpType
      // 2) T::PhpType          -- T::PhpType
      struct tl_tree_type *child = (struct tl_tree_type *)tree->children[0];
      if (is_php_array(child->type) || vk::any_of_equal(child->type->name, TL_INT, TL_DOUBLE, TL_FLOAT, TL_STRING, TL_LONG)) {
        // php_field_type::t_int    || php_field_type::t_double
        // php_field_type::t_string || php_field_type::t_array
        dst += make_tl_class_name(dst, "", "maybe", "", '\0');
        // if it's wrapped to Optional, we add "maybe" to name
      } else {
        // php_field_type::t_class   || php_field_type::t_mixed
        // php_field_type::t_boolean || php_field_type::t_maybe
        return get_full_tree_type_name(dst, child, NULL);
        // otherwise, skip current node and just go straight to the maybe inner
      }
    } else if (is_php_builtin(cur_type)) {
      // if cur node is builtin, add builtin type name to res
      if (cur_type->name == TL_INT || cur_type->name == TL_FLOAT || cur_type->name == TL_STRING) {
        dst += make_tl_class_name(dst, "", cur_type->constructors[0]->id, "", '\0');
      } else if (cur_type->name == TL_DOUBLE) {
        dst += make_tl_class_name(dst, "", "float", "", '\0');
      } else if (cur_type->name == TL_LONG || !strcmp(cur_type->id, "#")) {
        dst += make_tl_class_name(dst, "", "int", "", '\0');
      } else if (!strcmp(cur_type->id, "Bool")) {
        dst += make_tl_class_name(dst, "", "bool", "", '\0');
      } else {
        php_error_docref(NULL, E_ERROR,
                         "Unexpected type %s during creating instance", cur_type->id);
      }
      return dst - start_dst;
    } else if (is_tl_type_flat(cur_type, NULL)) {
      // the order is important, because types which are arrays in php can be flat (Tuple, IntKeyDictionary, ...)
      struct tl_tree *flat_inner = get_first_explicit_arg(cur_type->constructors[0])->type;
      if (TYPE(flat_inner) == NODE_TYPE_TYPE) {
        return get_full_tree_type_name(dst, (struct tl_tree_type *)flat_inner, NULL);
      }
      assert(TYPE(flat_inner) == NODE_TYPE_ARRAY); // Не поддерживаем шаблонные флат типы! (не может быть type_var)
      struct tl_tree_array *as_arr = (struct tl_tree_array *)flat_inner;
      assert(as_arr->args_num == 1); // запрещаем флатиться массивам, у которых ячейка длины не 1
      struct tl_tree *arr_flat_inner = as_arr->args[0]->type;
      assert(TYPE(arr_flat_inner) == NODE_TYPE_TYPE); // Не поддерживаем шаблонные флат типы! (не может быть type_var)
      const char *str_array = "array_";
      dst += make_tl_class_name(dst, "", str_array, "", '\0');
      return strlen(str_array) + get_full_tree_type_name(dst, (struct tl_tree_type *)arr_flat_inner, NULL);
    } else {
      // if it's ordinary tl type, add its name to res
      dst += make_tl_class_name(dst, "", cur_type->id, "", '_');
    }
  }
  // }}
  // At this part we make the rest part of name, by iterating through all children, e.g.
  // "__child1__child2__array_childOfArray__child3__maybe_int"
  for (int i = 0; i < tree->children_num; ++i) {
    struct tl_tree *child = tree->children[i];
    switch (TYPE(child)) {
      case NODE_TYPE_TYPE: {
        if (is_php_array(cur_type) || !strcmp(cur_type->id, "Maybe")) {
          strcpy(dst, "_");
          ++dst;
        } else {
          strcpy(dst, "__");
          dst += 2;
        }
        dst += get_full_tree_type_name(dst, (struct tl_tree_type *)child, NULL);
        break;
      }
      case NODE_TYPE_ARRAY:
      case NODE_TYPE_VAR_TYPE: {
        php_error_docref(NULL, E_ERROR,
                         "Unexpected node type %d during creating instance", TYPE(child));
        break;
      }
    }
  }
  return dst - start_dst;
}

/**
 * Generates class name for creating instance of this class.
 */
static void get_php_class_name(char *dst, struct tl_tree_type *tree, int num) {
  assert(tree);
  const char *constructor_name = tree->type->constructors[num]->id;
  if (!strcmp(tree->type->id, "ReqResult")) {
    if (!strcmp(constructor_name, "_")) {
      make_tl_class_name(dst, "", reqResult_underscore_class_name, "", '\0');
    } else if (!strcmp(constructor_name, "reqResultHeader")) {
      make_tl_class_name(dst, "", reqResult_header_class_name, "", '\0');
    } else if (!strcmp(constructor_name, "reqError")) {
      make_tl_class_name(dst, "", reqResult_error_class_name, "", '\0');
    } else {
      php_error_docref(NULL, E_WARNING,
                       "Unknown constructor of ReqResult: %s", constructor_name);
    }
  } else {
    static char php_prefix[PHP_PREFIX_BUFFER_LENGTH];
    get_php_prefix(php_prefix, constructor_name, TYPES);
    strcpy(dst, php_prefix);
    int prefix_len = strlen(php_prefix);
    dst += prefix_len;
    // tree->type isn't flat => constructor is correct
    int len = get_full_tree_type_name(dst, tree, constructor_name);
    check_buffer_overflow(prefix_len + len + 1);
  }
}

static zval *create_php_instance(const char *class_name) {
  //fprintf(stderr, "creating instance of class %s\n", class_name);
  zval *ci;
  VK_ALLOC_INIT_ZVAL (ci);
  zend_class_entry *ce = vk_get_class(class_name);
  object_init_ex(ci, ce);
  return ci;
}

static zval *make_query_result_or_error(zval **r, const char *error_msg, int error_code);

/**
 * This function extracts ORIGINAL tl name from given php class name.
 * It processes cases with changed names of php tl classes, such as: "rpcResponseOk" (original "_") and etc.
 */
void get_current_combinator_name(char *dst, const char *php_class_name) {
  if (!strcmp(php_class_name, reqResult_underscore_class_name)) {
    strcpy(dst, "_");
    return;
  } else if (!strcmp(php_class_name, reqResult_header_class_name)) {
    strcpy(dst, "reqResultHeader");
    return;
  } else if (!strcmp(php_class_name, reqResult_error_class_name)) {
    strcpy(dst, "reqError");
    return;
  }
  get_php_namespace_by_php_class_name(dst, php_class_name);
  int slashes_cnt = 0;
  // php_class_name is assumed to be either
  // VK\TL\(namespace)\(Types|Functions)\(namespace)_name or
  // VK\TL\_common\(Types|Functions)\name
  // therefore, we need to skip 4 slashes
  while (slashes_cnt < 4) {
    if (*php_class_name++ == '\\') {
      ++slashes_cnt;
    }
  }
  if (!strcmp(dst, php_common_namespace)) {
    strcpy(dst, php_class_name);
  } else {
    // we need to change '_' to '.'
    int namespace_len = strlen(dst);
    strcpy(dst, php_class_name);
    dst[namespace_len] = '.';
  }
  check_buffer_overflow(strlen(dst) + 1);
  // here dst looks like "graph.someComb__int" or "vectorTotal__int"
  // and we just need to drop part "__int"
  char *template_suffix_start = strstr(dst, "__");
  if (template_suffix_start != NULL) {
    int template_suffix_pos = template_suffix_start - dst;
    dst[template_suffix_pos] = 0;
  }
}

/* {{{ PHP arrays interaction */
VK_ZVAL_API_P get_field(zval *arr, const char *id, int num, zval **dst) {
  //fprintf(stderr, "@@@@@@@ get_field %s %d\n", id, num);
  ADD_CNT (get_field);
  START_TIMER (get_field);
  assert (arr);
//  fprintf (stderr, "arr = %p, type = %d\n", arr, (int)Z_TYPE_PP (arr));
  if (Z_TYPE_P (arr) != IS_ARRAY && Z_TYPE_P (arr) != IS_OBJECT) {
//    fprintf (stderr, "=(\n");
    END_TIMER (get_field);
    return 0;
  }

  VK_ZVAL_API_P t = NULL;
  if (id && strlen(id)) {
    switch (Z_TYPE_P (arr)) {
      case IS_ARRAY:
        t = vk_zend_hash_find(Z_ARRVAL_P (arr), id, VK_STR_API_LEN(strlen(id)));
        break;
      case IS_OBJECT:
        if (!strcmp(id, "_")) {
          char php_class_name[PHP_CLASS_NAME_BUFFER_LENGTH];
          vk_get_class_name(arr, php_class_name);
          char current_combinator_name[PHP_CLASS_NAME_BUFFER_LENGTH];
          get_current_combinator_name(current_combinator_name, php_class_name);
          //fprintf(stderr, "got _ = %s\n", current_combinator_name);
          VK_ALLOC_INIT_ZVAL(*dst);
          VK_ZVAL_STRING_DUP(*dst, current_combinator_name);
          //fprintf(stderr, "######### %s\n", current_combinator_name);
        } else {
          //fprintf(stderr, "reading instance property %s\n", id);
          if (strlen(id)) {
            *dst = vk_zend_read_public_property(arr, id);
          } else {
            const size_t arg_name_size = 30;
            char arg_name[arg_name_size];
            snprintf(arg_name, arg_name_size, "arg%d", num);
            *dst = vk_zend_read_public_property(arr, arg_name);
          }
        }
        t = VK_ZVALP_TO_API_ZVALP(*dst);
        break;
      default:
        t = 0;
    }
  }

  if (Z_TYPE_P(arr) == IS_OBJECT) {
    END_TIMER (get_field);
    return t;
  }

  if (t) {
    END_TIMER (get_field);
    return t;
  }

  t = vk_zend_hash_index_find(Z_ARRVAL_P (arr), num);
  if (t) {
    END_TIMER (get_field);
    return t;
  }
  END_TIMER (get_field);
  return 0;
}

void array_set_field(zval **arr, zval *val, const char *id, long long num) {
  ADD_CNT (array_set_field);
  START_TIMER (array_set_field);
  if (!*arr) {
    php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nAttempt to array_set_field() of NULL tl object", tl_current_function_name);
    VK_ALLOC_INIT_ZVAL (*arr);
    array_init (*arr);
  }
  assert (val);
  assert (*arr && Z_TYPE_P(*arr) == IS_ARRAY);
  #ifdef VLOG
  fprintf (stderr, "set_field: num:%d val_type:%d arr:%p\n", num, Z_TYPE_P (val), *arr);
  #endif
  if (id && strlen(id)) {
    vk_add_assoc_zval_nod (*arr, (char *)id, val);
  } else {
    vk_add_index_zval_nod (*arr, num, val);
  }
  END_TIMER (array_set_field);
}

void set_field(zval **arr, zval *val, const char *id, long long num) {
  //fprintf(stderr, "~~~ set_field %s %lld\n", id, num);
  ADD_CNT (set_field);
  START_TIMER (set_field);
  if (!*arr) {
    php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nAttempt to set_field() of NULL tl object", tl_current_function_name);
    VK_ALLOC_INIT_ZVAL (*arr);
    array_init (*arr);
  }
  switch (typed_mode) {
    case 0: {
      assert (val);
      assert (*arr && Z_TYPE_P(*arr) == IS_ARRAY);
      #ifdef VLOG
      fprintf (stderr, "set_field: num:%d val_type:%d arr:%p\n", num, Z_TYPE_P (val), *arr);
      #endif
      if (id && strlen(id)) {
        vk_add_assoc_zval_nod (*arr, (char *)id, val);
      } else {
        vk_add_index_zval_nod (*arr, num, val);
      }
      break;
    }
    case 1: {
      assert (val);
      assert (*arr && Z_TYPE_P(*arr) == IS_OBJECT);
      #ifdef VLOG
      fprintf (stderr, "set_field: num:%d val_type:%d arr:%p\n", num, Z_TYPE_P (val), *arr);
      #endif
      if (id && strlen(id)) {
        vk_zend_update_public_property_nod(*arr, id, val);
      } else {
        const size_t arg_name_size = 10;
        char arg_name[arg_name_size];
        snprintf(arg_name, arg_name_size, "arg%lld", num);
        vk_zend_update_public_property_nod(*arr, arg_name, val);
      }
      break;
    }
  }
  END_TIMER (set_field);
}

void set_field_string(zval **arr, char *val, const char *id, int num) {
  ADD_CNT (set_field_string);
  START_TIMER (set_field_string);
  if (!*arr) {
    php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nAttempt to set_field_string() of NULL tl object", tl_current_function_name);
    VK_ALLOC_INIT_ZVAL (*arr);
    array_init (*arr);
  }
  switch (typed_mode) {
    case 0:
      assert (val);
      assert (*arr && Z_TYPE_P(*arr) == IS_ARRAY);
      if (id && strlen(id)) {
        vk_add_assoc_string_dup (*arr, (char *)id, val);
      } else {
        vk_add_index_string_dup (*arr, num, val);
      }
      break;
    case 1:
      assert (val);
      assert (*arr && Z_TYPE_P(*arr) == IS_OBJECT);
      if (id && strlen(id)) {
        vk_zend_update_public_property_string(*arr, id, val);
      } else {
        const size_t arg_name_size = 10;
        char arg_name[arg_name_size];
        snprintf(arg_name, arg_name_size, "arg%d", num);
        vk_zend_update_public_property_string(*arr, arg_name, val);
      }
      break;
  }
  END_TIMER (set_field_string);
}

void set_field_int(zval **arr, int val, const char *id, int num) {
  ADD_CNT (set_field_int);
  START_TIMER (set_field_int);
  if (!*arr) {
    php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nAttempt to set_field_int() of NULL tl object", tl_current_function_name);
    VK_ALLOC_INIT_ZVAL (*arr);
    array_init (*arr);
  }
  switch (typed_mode) {
    case 0:
      if (id && strlen(id)) {
        add_assoc_long (*arr, (char *)id, val);
      } else {
        add_index_long(*arr, num, val);
      }
      break;
    case 1:
      if (id && strlen(id)) {
        vk_zend_update_public_property_long(*arr, id, val);
      } else {
        const size_t arg_name_size = 10;
        char arg_name[arg_name_size];
        snprintf(arg_name, arg_name_size, "arg%d", num);
        vk_zend_update_public_property_long(*arr, arg_name, val);
      }
      break;
  }
  END_TIMER (set_field_int);
}

int get_array_size(zval **arr) {
  return zend_hash_num_elements (Z_ARRVAL_P(*arr));
}
/* }}} */

static bool wrap_to_function_result_if_needed(zval **tl_obj_fun_res, const char *tl_fun_name, bool dup) {
  // if dup is false, *tl_obj_fun_res is a kind of rvalue and its duplication will lead to memory leak
  assert(*tl_obj_fun_res);
  // if it's already a function result no wrapping is needed (multi exclamation optimization)
  if (Z_TYPE_P(*tl_obj_fun_res) == IS_OBJECT) {
    zend_class_entry *class_entry = Z_OBJCE_P(*tl_obj_fun_res);
    if (class_entry->num_interfaces == 1 && !strcmp(VK_ZSTR_VAL(class_entry->interfaces[0]->name), rpcFunctionReturnResult_class_name)) {
      return false;
    }
  }
  static char class_name[PHP_CLASS_NAME_BUFFER_LENGTH];
  static char php_prefix[PHP_PREFIX_BUFFER_LENGTH];
  get_php_prefix(php_prefix, tl_fun_name, FUNCTIONS);
  check_buffer_overflow(make_tl_class_name(class_name, php_prefix, tl_fun_name, "_result", '_'));
  zval *f_result_wrapper = create_php_instance(class_name);
  if (dup) {
    vk_zend_update_public_property_dup(f_result_wrapper, "value", *tl_obj_fun_res);
  } else {
    vk_zend_update_public_property_nod(f_result_wrapper, "value", *tl_obj_fun_res);
  }
  *tl_obj_fun_res = f_result_wrapper;
  return true;
}

#define use_var_nat_full_form(x) 1

long long var_nat_const_to_int(void *x) {
  if (((long)x) & 1) {
    return (((long)x) + 0x80000001l) / 2;
  } else {
    return ((struct tl_tree_nat_const *)x)->value;
  }
}

void *int_to_var_nat_const(long long x) {
  if (use_var_nat_full_form (x)) {
    auto *T = reinterpret_cast<tl_tree_nat_const *>(zzemalloc(sizeof(tl_tree_nat_const)));
    T->self.ref_cnt = 1;
    T->self.flags = 0;
    T->self.methods = &tl_nat_const_full_methods;
#ifdef VLOG
    fprintf(stderr, "Creating nat const full %lld at %p\n", x, T);
#endif
    T->value = x;
    total_ref_cnt++;
    dynamic_tree_nodes++;
    total_tree_nodes_existed++;
    return T;
  } else {
    return (void *)(long)(x * 2 - 0x80000001l);
  }
}

void *int_to_var_nat_const_init(long long x) {
  if (use_var_nat_full_form (x)) {
    tl_tree_nat_const *T = reinterpret_cast<tl_tree_nat_const *>(zzmalloc(sizeof(*T)));
    ADD_PMALLOC (sizeof(*T));
    T->self.ref_cnt = 1;
    T->self.flags = 0;
    T->self.methods = &tl_pnat_const_full_methods;
    T->value = x;
    total_ref_cnt++;
    persistent_tree_nodes++;
    total_tree_nodes_existed++;
    return T;
  } else {
    return (void *)(long)(x * 2 - 0x80000001l);
  }
}


int get_constructor(struct tl_type *t, const char *id) {
  int i;
  for (i = 0; i < t->constructors_num; i++) {
    if (!strcmp(t->constructors[i]->id, id)) {
      return i;
    }
  }
  return -1;
}

int get_constructor_by_name(struct tl_type *t, int name) {
  for (int i = 0; i < t->constructors_num; i++) {
    if (t->constructors[i]->name == static_cast<unsigned int>(name)) {
      return i;
    }
  }
  return -1;
}

#define MAX_VARS 100000
struct tl_tree *__vars[MAX_VARS];
struct tl_tree **last_var_ptr;

struct tl_tree **get_var_space(struct tl_tree **vars, int n) {
//  fprintf (stderr, "get_var_space: %d\n", n);
  if (vars - n < __vars) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_NOT_ENOUGH_MEMORY, "Not enough memory");
    return 0;
  }
  if (last_var_ptr > vars - n) {
    last_var_ptr = vars - n;
  }
  int i;
  for (i = -n; i < 0; i++) {
    if (vars[i])
      DEC_REF(vars[i]);
  }
  memset(vars - n, 0, sizeof(*vars) * n);

  return vars - n;
}


void tl_parse_on_rinit() {
  last_var_ptr = __vars + MAX_VARS;
}

void tl_parse_on_rshutdown() {
  while (last_var_ptr < __vars + MAX_VARS) {
    if (*last_var_ptr) {
      DEC_REF (*last_var_ptr);
      *last_var_ptr = 0;
    }
    last_var_ptr++;
  }
}

#define MAX_SIZE 100000
void *_Data[MAX_SIZE];

typedef void *(*fpr_t)(void **IP, void **Data, zval **arr, struct tl_tree **vars);
#define TLUNI_NEXT return (*(fpr_t *) IP) (IP + 1, Data, arr, vars);
#define TLUNI_START(IP, Data, arr, vars) (*(fpr_t *) IP) (IP + 1, Data, arr, vars)

#define TLUNI_OK (void *)1l
#define TLUNI_FAIL (void *)0

void **fIP;

inline void tl_debug(const char *s __attribute__((unused)), int n __attribute__((unused))) {
  //fprintf(stderr, "%s\n", s);
}

/* {{{ Interface functions */

struct tl_tree *store_function(VK_ZVAL_API_P arr) {
  ADD_CNT(store_function)
  START_TIMER(store_function)
  assert (arr);
  zval *dst;
  // IN PHP5 r == &dst => lifetime is limited by scope of this function
  VK_ZVAL_API_P r = get_field(VK_ZVAL_API_TO_ZVALP(arr), "_", 0, &dst);
  if (!r) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_NO_CONSTRUCTOR, "Can't store function with no name");
    END_TIMER(store_function)
    return 0;
  }
  struct tl_combinator *c;
  char *s = 0;
  int l = 0;
  if (VK_Z_API_TYPE (r) == IS_STRING) {
    int l;
    s = parse_zend_string(r, &l);
    c = tl_fun_get_by_id(s);
  } else {
    l = parse_zend_long(r);
    c = tl_fun_get_by_name(l);
  }
  if (!c) {
    #ifdef VLOG
    if (Z_TYPE_PP (r) == IS_STRING) {
      fprintf (stderr, "Function %s not found\n", s);
    } else {
      fprintf (stderr, "Function %d not found\n", l);
    }
    #endif
    vkext_reset_error();
    if (VK_Z_API_TYPE (r) == IS_STRING) {
      vkext_error_format(VKEXT_ERROR_UNKNOWN_CONSTRUCTOR, "Function %s not found", s);
    } else {
      vkext_error_format(VKEXT_ERROR_UNKNOWN_CONSTRUCTOR, "Function %d not found", l);
    }
    if (VK_Z_API_TYPE (arr) == IS_OBJECT) {
      zval_ptr_dtor(r);
      efree(dst);
    }
    END_TIMER(store_function)
    return 0;
  }
  if (!allow_internal_rpc_queries && (c->flags & COMBINATOR_FLAG_INTERNAL)) {
    php_error_docref(NULL, E_WARNING, "### DEPRECATED TL FEATURE ###:\nStoring of internal tl function %s\n", c->id);
    if (VK_Z_API_TYPE (arr) == IS_OBJECT) {
      zval_ptr_dtor(r);
      efree(dst);
    }
    END_TIMER(store_function)
    return 0;
  }
#ifdef VLOG
  fprintf (stderr, "Storing functions %s\n", c->id);
#endif
  tl_current_function_name = c->id;
  struct tl_tree **vars = get_var_space(__vars + MAX_VARS, c->var_num);
  if (!vars) {
    vkext_reset_error();
    if (VK_Z_API_TYPE (r) == IS_STRING) {
      vkext_error_format(VKEXT_ERROR_NOT_ENOUGH_MEMORY, "Not enough memory to store %s", s);
    } else {
      vkext_error_format(VKEXT_ERROR_NOT_ENOUGH_MEMORY, "Not enough memory to store %d", l);
    }
    if (VK_Z_API_TYPE (arr) == IS_OBJECT) {
      zval_ptr_dtor(r);
      efree(dst);
    }
    END_TIMER(store_function)
    return 0;
  }
  static zval *_arr[MAX_DEPTH];
  _arr[0] = VK_ZVAL_API_TO_ZVALP(arr);
  void *res = TLUNI_START (c->IP, _Data, _arr, vars);
#ifdef VLOG
  if (res) {
    void *T = res;
    fprintf (stderr, "Store end: T->id = %s, T->ref_cnt = %d, T->flags = %d\n", ((struct tl_tree_type *)T)->type->id, ((struct tl_tree_type *)T)->self.ref_cnt, ((struct tl_tree_type *)T)->self.flags);
  }
#endif
  if (res) {
    tl_tree_type *T = reinterpret_cast<tl_tree_type *>(zzemalloc(sizeof(*T)));
    T->self.flags = 0;
    T->self.ref_cnt = 1;
    T->self.methods = &tl_type_methods;
    T->type = tl_type_get_by_id("ReqResult");
    T->children_num = 1;
    T->children = reinterpret_cast<tl_tree **>(zzemalloc(sizeof(*T->children)));
    *T->children = reinterpret_cast<tl_tree *>(res);
    res = T;
    dynamic_tree_nodes++;
    total_ref_cnt++;
  } else {
    if (VK_Z_API_TYPE (r) == IS_STRING) {
      vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Failed to store %s", s);
    } else {
      vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Failed to store %d", l);
    }
  }
//  assert (((struct tl_tree *)res)->ref_cnt > 0);
  if (VK_Z_API_TYPE (arr) == IS_OBJECT) {
    zval_ptr_dtor(r);
    efree(dst);
  }
  END_TIMER(store_function)
  return reinterpret_cast<tl_tree *>(res);
}

zval **fetch_function(struct tl_tree *T) {
  ADD_CNT(fetch_function)
  START_TIMER(fetch_function)
#ifdef VLOG
  int *cptr;
  for (cptr = (int *)inbuf->rptr; cptr < (int *)inbuf->wptr; cptr ++) {
    fprintf (stderr, "Int %d (%08x)\n", *cptr, *cptr);
  }
#endif
  assert (T);
  struct tl_tree **vars = __vars + MAX_VARS;
  static zval *_arr[MAX_DEPTH];
  *_arr = 0;
  *(_Data) = T;
#ifdef VLOG
  fprintf (stderr, "Fetch begin: T->id = %s, T->ref_cnt = %d, T->flags = %d\n", ((struct tl_tree_type *)T)->type->id, ((struct tl_tree_type *)T)->self.ref_cnt, ((struct tl_tree_type *)T)->self.flags);
#endif
//  INC_REF (T);
  int x = do_rpc_lookup_int(NULL);
  if (x == TL_RPC_REQ_ERROR) {
    assert (tl_parse_int() == TL_RPC_REQ_ERROR);
    tl_parse_long();
    int error_code = tl_parse_int();
    char *s = 0;
    int l = tl_parse_string(&s);
//    fprintf (stderr, "Error_code %d: error %.*s\n", error_code, l, s);
    *_arr = make_query_result_or_error(NULL, l >= 0 ? s : "unknown", error_code);
    if (s) {
      free(s);
    }
    DEC_REF (T);

    END_TIMER(fetch_function)
    return _arr;
  }
  void *res = TLUNI_START (fIP, _Data + 1, _arr, vars);
//  DEC_REF (T);
  if (res == TLUNI_OK) {
    tl_parse_end();
    if (tl_parse_error()) {
      res = TLUNI_FAIL;
    }
  }
  END_TIMER(fetch_function)
  if (res == TLUNI_OK) {
    if (!*_arr) {
      VK_ALLOC_INIT_ZVAL(*_arr);
      ZVAL_BOOL (*_arr, 1);
    }
    return _arr;
  } else {
    if (*_arr) {
      zval_dtor (*_arr);
    }
    *_arr = make_query_result_or_error(NULL, "Can't parse response", TL_ERROR_RESPONSE_SYNTAX);
    return _arr;
  }
}

void _extra_dec_ref(struct rpc_query *q) {
  if (q->extra) {
    total_tl_working--;
  }
  DEC_REF (q->extra);
  q->extra = 0;
  q->extra_free = 0;
}

struct rpc_query *vk_memcache_query_one(struct rpc_connection *c, double timeout, VK_ZVAL_API_P arr, int ignore_answer) {
  do_rpc_clean();
  START_TIMER (tmp);
  void *res = store_function(arr);
  END_TIMER (tmp);
  if (!res) {
    return 0;
  }
  struct rpc_query *q;
  if (!(q = do_rpc_send_noflush(c, timeout, ignore_answer))) {
    DEC_REF (res);
    vkext_error(VKEXT_ERROR_NETWORK, "Can't send packet");
    return 0;
  }
  if (q == (struct rpc_query *)1) { // answer is ignored
    assert (ignore_answer);
    DEC_REF (res);
    return q;
  }
  assert (!ignore_answer);
  q->extra = res;
  q->extra_free = _extra_dec_ref;
  total_tl_working++;
  return q;
}

zval **vk_memcache_query_result_one(struct tl_tree *T) {
  tl_parse_init();
  START_TIMER (tmp);
  zval **r = fetch_function(T);
  //fprintf(stderr, "~~~~ after fetch:\n");
  //php_debug_zval_dump(*r, 1);
  END_TIMER (tmp);
  return r;
}

void vk_memcache_query_many(struct rpc_connection *c, VK_ZVAL_API_P arr, double timeout, zval **r, int ignore_answer) {
  VK_ZVAL_API_P zkey;
  array_init (*r);
  unsigned long index;
  NEW_INIT_Z_STR_P(key);

  VK_ZEND_HASH_FOREACH_KEY_VAL(VK_Z_API_ARRVAL(arr), index, key, zkey) {
    if (VK_ZSTR_P_NON_EMPTY(key)) {
      index = 0;
    }
    struct rpc_query *q = vk_memcache_query_one(c, timeout, zkey, ignore_answer);
    if (VK_ZSTR_P_NON_EMPTY(key)) {
      if (q) {
        if (ignore_answer) {
          add_assoc_long_ex(*r, key->val, key->len, -1);
        } else {
          add_assoc_long_ex(*r, key->val, key->len, q->qid);
        }
      } else {
        add_assoc_long_ex(*r, key->val, key->len, 0);
      }
    } else {
      if (q) {
        if (ignore_answer) {
          add_index_long(*r, index, -1);
        } else {
          add_index_long(*r, index, q->qid);
        }
      } else {
        add_index_bool(*r, index, 0);
      }
    }
  } VK_ZEND_HASH_FOREACH_END();
}

long long vk_queries_count(INTERNAL_FUNCTION_PARAMETERS) {
  extern long long total_working_qid;
  return total_working_qid;
}

void vk_memcache_query(INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[5];
  if (argc < 2) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_INVALID_CALL, "Not enough parameters for rpc_tl_query");
    END_TIMER (parse);
    RETURN_EMPTY_ARRAY();
  }
  if (zend_get_parameters_array_ex (argc > 4 ? 4 : argc, z) == FAILURE) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_INVALID_CALL, "Can't parse parameters for rpc_tl_query");
    END_TIMER (parse);
    RETURN_EMPTY_ARRAY();
  }

  int fd = parse_zend_fd(VK_ZVAL_ARRAY_TO_API_P(z[0]));
  if (fd == VK_INCORRECT_FD) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_WRONG_TYPE, "Invalid connection type");
    END_TIMER (parse);
    RETURN_EMPTY_ARRAY();
  }

  struct rpc_connection *c = rpc_connection_get(fd);
//  if (!c || !c->server || c->server->status != rpc_status_connected) {
  if (!c || !c->servers) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_NETWORK, "Invalid connection in rpc_tl_query");
    END_TIMER (parse);
    RETURN_EMPTY_ARRAY();
  }

  double timeout = argc > 2 ? parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[2])) : c->default_query_timeout;
  int ignore_answer = argc > 3 ? parse_zend_bool(VK_ZVAL_ARRAY_TO_API_P(z[3])) : 0;
  update_precise_now();
  timeout += precise_now;

  if (VK_Z_API_TYPE (VK_ZVAL_ARRAY_TO_API_P(z[1])) != IS_ARRAY) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_INVALID_CALL, "Can't parse parameters for rpc_tl_query");
    RETURN_EMPTY_ARRAY();
  }
  END_TIMER (parse);

  vk_memcache_query_many(c, VK_ZVAL_ARRAY_TO_API_P(z[1]), timeout, &return_value, ignore_answer);
//  if (do_rpc_flush_server (c->server, timeout) < 0) {
  if (do_rpc_flush(timeout) < 0) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_NETWORK, "Can't send query");
    zval_dtor (return_value);
    RETURN_EMPTY_ARRAY();
  }
}

void vk_memcache_query1(INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[5];
  if (argc < 2) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_INVALID_CALL, "Not enough parameters for rpc_tl_query_one");
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (zend_get_parameters_array_ex (argc > 3 ? 3 : argc, z) == FAILURE) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_INVALID_CALL, "Can't parse parameters for rpc_tl_query_one");
    END_TIMER (parse);
    RETURN_FALSE;
  }

  int fd = parse_zend_fd(VK_ZVAL_ARRAY_TO_API_P(z[0]));
  struct rpc_connection *c = rpc_connection_get(fd);
  if (!c || !c->servers) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_NETWORK, "Invalid connection in rpc_tl_query");
    END_TIMER (parse);
    RETURN_FALSE;
  }

  double timeout = argc > 2 ? parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[2])) : c->default_query_timeout;
  update_precise_now();
  timeout += precise_now;

  END_TIMER (parse);

  struct rpc_query *q = vk_memcache_query_one(c, timeout, VK_ZVAL_ARRAY_TO_API_P(z[1]), 0);
  if (do_rpc_flush(timeout) < 0) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_NETWORK, "Can't send query");
    zval_dtor (return_value);
    RETURN_FALSE;
  }
  if (q) {
    RETURN_LONG(q->qid);
  } else {
    RETURN_LONG(0);
  }
}

static zval *make_query_result_or_error(zval **r, const char *error_msg, int error_code) {
  if (r) {
    return *r;
  }
  zval *_err;
  switch (typed_mode) {
    case 0: {
      VK_ALLOC_INIT_ZVAL (_err);
      array_init (_err);
      char *x = estrdup (error_msg);
      set_field_string(&_err, x, "__error", 0);
      efree (x);
      set_field_int(&_err, error_code, "__error_code", 0);
      break;
    }
    case 1: {
      _err = create_php_instance(reqResult_error_class_name);
      char *_x = estrdup (error_msg);
      set_field_string(&_err, _x, "error", 0);
      efree (_x);
      set_field_int(&_err, error_code, "error_code", 0);
      break;
    }
  }
  return _err;
}

void vk_memcache_query_result_many(struct rpc_queue *Q, double timeout, zval **r) {
  array_init (*r);
  while (!do_rpc_queue_empty(Q)) {
    long long qid = do_rpc_queue_next(Q, timeout);
    if (qid <= 0) {
      return;
    }
    struct rpc_query *q = rpc_query_get(qid);
    tl_tree *T = reinterpret_cast<tl_tree *>(q->extra);
    tl_current_function_name = q->fun_name;
    INC_REF (T);

    if (do_rpc_get_and_parse(qid, timeout - precise_now) < 0) {
      continue;
    }
    zval *res = make_query_result_or_error(vk_memcache_query_result_one(T),
                                           "Response not found, probably timed out",
                                           TL_ERROR_RESPONSE_NOT_FOUND);
    vk_add_index_zval_nod (*r, qid, res);
  }
}

void vk_memcache_query_result1(INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[5];
  if (argc < 1) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (zend_get_parameters_array_ex (argc > 1 ? 2 : argc, z) == FAILURE) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  long long qid = parse_zend_long(VK_ZVAL_ARRAY_TO_API_P(z[0]));
  struct rpc_query *q = rpc_query_get(qid);
  if (!q) {
    zval *r = make_query_result_or_error(NULL, "No answer received or duplicate/wrong query_id",
                                         TL_ERROR_WRONG_QUERY_ID);
    RETVAL_ZVAL(r, false, true);
    efree(r);
    return;
  }
  tl_current_function_name = q->fun_name;
  update_precise_now();
  double timeout = (argc < 2) ? q->timeout : precise_now + parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[1]));
  END_TIMER (parse);
  auto *T = reinterpret_cast<tl_tree *>(q->extra);
  INC_REF (T);
  if (do_rpc_get_and_parse(qid, timeout - precise_now) < 0) {
    zval *r = make_query_result_or_error(NULL, "Response not found, probably timed out",
                                         TL_ERROR_RESPONSE_NOT_FOUND);
    RETVAL_ZVAL(r, false, true);
    efree(r);
    return;
  }
  zval *r = make_query_result_or_error(vk_memcache_query_result_one(T), "Response not found, probably timed out",
                                       TL_ERROR_RESPONSE_NOT_FOUND);
  RETVAL_ZVAL(r, false, true);
  efree(r);
}

void vk_memcache_query_result(INTERNAL_FUNCTION_PARAMETERS) {
  ADD_CNT (parse);
  START_TIMER (parse);
  int argc = ZEND_NUM_ARGS ();
  VK_ZVAL_API_ARRAY z[5];
  if (argc < 1) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  if (zend_get_parameters_array_ex (argc > 1 ? 2 : argc, z) == FAILURE) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  VK_ZVAL_API_P arr = VK_ZVAL_ARRAY_TO_API_P(z[0]);
  if (VK_Z_API_TYPE (arr) != IS_ARRAY) {
    RETURN_FALSE;
  }

  VK_ZVAL_API_P zkey;

  int max_size = zend_hash_num_elements (VK_Z_API_ARRVAL(arr));
  long long qids[max_size];
  int size = 0;

  VK_ZEND_HASH_FOREACH_VAL(VK_Z_API_ARRVAL(arr), zkey) {
    assert (size < max_size);
    qids[size++] = parse_zend_long(zkey);
    if (!qids[size - 1]) {
      size--;
    } else {
      struct rpc_query *q = rpc_query_get(qids[size - 1]);
      if (q && q->queue_id) {
        delete_query_from_queue_ex(q);
      }
    }
  } VK_ZEND_HASH_FOREACH_END();

  long long Qid = do_rpc_queue_create(size, qids);
  if (!Qid) {
    END_TIMER (parse);
    RETURN_FALSE;
  }
  struct rpc_queue *Q = rpc_queue_get(Qid);
  assert (Q);
  update_precise_now();
  double timeout = (argc < 2) ? Q->timeout : precise_now + parse_zend_double(VK_ZVAL_ARRAY_TO_API_P(z[1]));
  END_TIMER (parse);

  zval *R = NULL;
  VK_ALLOC_INIT_ZVAL(R);
  vk_memcache_query_result_many(Q, timeout, &R);

  array_init (return_value);
  NEW_INIT_Z_STR_P(key);
  unsigned long index;

  VK_ZEND_HASH_FOREACH_KEY_VAL(VK_Z_API_ARRVAL(arr), index, key, zkey) {
    if (VK_ZSTR_P_NON_EMPTY(key)) {
      index = 0;
    }

    long long qid = parse_zend_long(zkey);
    if (qid > 0) {
      VK_ZVAL_API_P r;
      if ((r = vk_zend_hash_index_find(Z_ARRVAL_P (R), qid)) == NULL) {
        r = 0;
      } else {
        VK_ZAPI_TRY_ADDREF_P (r);
      }
      if (VK_ZSTR_P_NON_EMPTY(key)) {
        if (r) {
          add_assoc_zval_ex(return_value, key->val, key->len, VK_ZVAL_API_TO_ZVALP(r));
        } else {
          zval *_err = make_query_result_or_error(NULL, "Response not found, probably timed out", TL_ERROR_RESPONSE_NOT_FOUND);
          vk_add_assoc_zval_ex_nod (return_value, key->val, key->len, _err);
        }

      } else {
        if (r) {
          add_index_zval(return_value, index, VK_ZVAL_API_TO_ZVALP(r));
        } else {
          zval *_err = make_query_result_or_error(NULL, "Response not found, probably timed out", TL_ERROR_RESPONSE_NOT_FOUND);
          vk_add_index_zval_nod (return_value, index, _err);
        }
      }
    }

  } VK_ZEND_HASH_FOREACH_END();

  zval_dtor (R);
  efree (R);
  do_rpc_queue_free(Qid);
}
/* }}} */

/**** Simple code functions {{{ ****/

void *tls_push(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  *(Data++) = *(IP++);
  INC_REF (*(Data - 1));
  TLUNI_NEXT;
}

void *tls_dup(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  void *x = *(Data - 1);
  *(Data++) = x;
  TLUNI_NEXT;
}

void *tls_pop(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  --Data;
  TLUNI_NEXT;
}

void *tls_arr_pop(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  --arr;
  TLUNI_NEXT;
}

void *tls_arr_push(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  ++arr;
  *arr = 0;
  TLUNI_NEXT;
}

void *tls_ret(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  return *(IP++);
}

void *tls_store_int(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  do_rpc_store_int((long)*(IP++));
  TLUNI_NEXT;
}

/*** }}} */

/**** Combinator store code {{{ ****/


void *tlcomb_store_const_int(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  do_rpc_store_int((long)*(IP++));
  TLUNI_NEXT;
}

void *tlcomb_skip_const_int(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int a = tl_parse_int();
  if (tl_parse_error()) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_WRONG_TYPE, "Can't parse int");
    return 0;
  }
  if (a != (long)*(IP++)) {
    vkext_reset_error();
    vkext_error_format(VKEXT_ERROR_WRONG_VALUE, "Expected %ld found %d", (long)*(IP - 1), a);
    return 0;
  }
  TLUNI_NEXT;
}

/****
 *
 * Data [data] => [data] result
 * IP
 *
 ****/

void *tlcomb_store_any_function(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  zval *dst;
  // IN PHP5 r == &dst => lifetime is limited by scope of this function
  VK_ZVAL_API_P r = get_field(*arr, "_", 0, &dst);
  if (!r) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_NO_CONSTRUCTOR, "Can't store function with no name");
    return 0;
  }
  int l;
  char *s = parse_zend_string(r, &l);
  struct tl_combinator *c = tl_fun_get_by_id(s);
#ifdef VLOG
  fprintf (stderr, "Storing functions %s\n", c->id);
#endif
  if (!c) {
    vkext_reset_error();
    vkext_error_format(VKEXT_ERROR_UNKNOWN_CONSTRUCTOR, "Unknown function \"%s\"", s);
    if (Z_TYPE_P(*arr) == IS_OBJECT) {
      zval_ptr_dtor(r);
      efree(dst);
    }
    return 0;
  }
  struct tl_tree **new_vars = get_var_space(vars, c->var_num);
  if (!new_vars) {
    vkext_reset_error();
    vkext_error_format(VKEXT_ERROR_NOT_ENOUGH_MEMORY, "Not enough memory [function %s]", s);
    if (Z_TYPE_P(*arr) == IS_OBJECT) {
      zval_ptr_dtor(r);
      efree(dst);
    }
    return 0;
  }
  void *res = TLUNI_START (c->IP, Data, arr, new_vars);
  if (!res) {
    vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Can't store arguments of %s", s);
    if (Z_TYPE_P(*arr) == IS_OBJECT) {
      zval_ptr_dtor(r);
      efree(dst);
    }
    return 0;
  }
  *(Data++) = res;

  // store the !X (internal query) to be used in tlcomb_fetch_type()
  auto *e = reinterpret_cast<tl_tree *>(res);
  if (!(e->flags & FLAG_FORWARDED)) {
    e->forwarded_fun_name = c->id;
    e->flags |= FLAG_FORWARDED;
  }

  if (Z_TYPE_P(*arr) == IS_OBJECT) {
    zval_ptr_dtor(r);
    efree(dst);
  }
  TLUNI_NEXT;
}

/****
 *
 * Data [data] result => [data]
 * IP
 *
 ****/
 /*
  * This function is called on fetching of any tl type. It fetches it to *arr.
  */
void *tlcomb_fetch_type(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  auto *e = *reinterpret_cast<tl_tree_type **>(--Data);
  assert (e);
  struct tl_type *t = e->type;
  assert (t);
   //fprintf(stderr, "^^^^^^^^^^^^ Fetching type %s. Flags %d\n", t->id, e->self.flags);
#ifdef VLOG
  fprintf (stderr, "Fetching type %s. Flags %d\n", t->id, e->self.flags);
#endif
  int n = -1;
  if (e->self.flags & FLAG_BARE) {
    if (t->constructors_num == 1) {
      n = 0;
    }
  } else {
    int p = tl_parse_save_pos();
    int x = tl_parse_int();
    if (tl_parse_error()) {
      DEC_REF (e);
      return 0;
    }
    #ifdef VLOG
    fprintf (stderr, "cosntructor name = %08x\n", x);
    #endif
    n = get_constructor_by_name(t, x);
    if (n < 0) {
      #ifdef VLOG
      fprintf (stderr, "Checking for default constructor\n");
      #endif
      if (t->flags & FLAG_DEFAULT_CONSTRUCTOR) {
        n = t->constructors_num - 1;
        tl_parse_restore_pos(p);
      }
    }
    if (n < 0) {
      DEC_REF (e);
      return 0;
    }
  }
  if (n >= 0) {
#ifdef VLOG
    fprintf (stderr, "Fetching constructor %s\n", t->constructors[n]->id);
#endif
    *(Data) = e;
    *arr = 0;
    // we create a storage for fetching. It's either array or instance of class.
    // Tuple, Dictionary, IntKeyDictionary, LongKeyDictionary are processed in special combs, not here. It's important, because they are flat...
    if (!is_tl_type_flat(t, NULL) && !is_php_builtin(t) && !is_php_array(t)) {
      // special case in gen_constructor_fetch() + tuple == builtin || array
      if (typed_mode) {
        // In typed mode we need to get full class name by traversing tl_tree of fetched expression (e) at first
        char class_name[PHP_CLASS_NAME_BUFFER_LENGTH];
        get_php_class_name(class_name, e, n);
        if (!strcmp(class_name, true_type_class_name)) {
          VK_ALLOC_INIT_ZVAL (*arr);
          ZVAL_TRUE (*arr);
        } else {
          *arr = create_php_instance(class_name);
        }
      } else {
        VK_ALLOC_INIT_ZVAL (*arr);
        array_init (*arr);
      }
    }
    struct tl_tree **new_vars = get_var_space(vars, t->constructors[n]->var_num);
    if (!new_vars) {
      return 0;
    }
    //fprintf(stderr, "^^^^^ call constructor %s\n", t->constructors[n]->id);
    void *res = TLUNI_START (t->constructors[n]->fIP, Data + 1, arr, new_vars); // here the fetching is done
    if (res != TLUNI_OK) {
      return 0;
    }
    if (!typed_mode && !(e->self.flags & FLAG_BARE) && (t->constructors_num > 1) && !(t->flags & FLAG_NOCONS)) {
      set_field_string(arr, t->constructors[n]->id, "_", 0);
    }
    if (typed_mode) {
      // In typed mode we need to add some wrappers that were not at nontyped mode
      // For example, we're fetching "rpcProxyDiagonal (memcache.get)"
      if (e->self.flags & FLAG_FORWARDED) {
        // if it's forwarded via exclamation.
        // before wrapping *arr :: memcache_strvalue
        wrap_to_function_result_if_needed(arr, e->self.forwarded_fun_name, false);
        // After wrapping
        // *arr :: memcache_get_result
        // ----------- $value :: memcache_strvalue
      } else if (!strcmp(e->type->constructors[n]->id, "_") || !strcmp(e->type->constructors[n]->id, "reqResultHeader")) {
        // if we're at the very end of fetching response
        // before wrapping
        // *arr :: RpcResponseOk
        // ----------- $result :: memcache_get_result[]
        const char *result_field_name = "result";
        zval *result = vk_zend_read_public_property(*arr, result_field_name);
        bool is_wrapped = wrap_to_function_result_if_needed(&result, tl_current_function_name, true); // dup is true, because result do has 2 owners here:
                                                                                                      // the first is *arr
                                                                                                      // the second is newly created wrapper of result
        if (is_wrapped) {
          vk_zend_update_public_property_nod(*arr, result_field_name, result);                        // and here the first owner disappears
        }
        // after wrapping
        // *arr :: RpcResponseOk
        // ----------- $result :: rpcProxyDiagonal_result
        // -------------------------- $value :: memcache_get_result[]
      }
    }
    DEC_REF (e);
    TLUNI_NEXT;
  } else {
#ifdef VLOG
    fprintf (stderr, "Fetching any constructor\n");
#endif
    php_error_docref(NULL, E_WARNING,
                     "### DEPRECATED TL FEATURE ###: "
                     "The constructor name must be given if type has several ones. It's going to guess the constructor in runtime (fetching phase) ! "
                     "Type: %s; "
                     "Function: %s",
                     t->id, tl_current_function_name);
    int k = tl_parse_save_pos();
    for (n = 0; n < t->constructors_num; n++) {
      *(Data) = e;
      struct tl_tree **new_vars = get_var_space(vars, t->constructors[n]->var_num);
      if (!new_vars) {
        return 0;
      }
      void *r = TLUNI_START (t->constructors[n]->IP, Data + 1, arr, new_vars);
      if (r == TLUNI_OK) {
        if (!(e->self.flags & FLAG_BARE) && !(t->flags & FLAG_NOCONS)) {
          set_field_string(arr, t->constructors[n]->id, "_", 0);
        }
        DEC_REF (e);
        TLUNI_NEXT;
      }
      assert (tl_parse_restore_pos(k));
      tl_parse_clear_error();
    }
    DEC_REF (e);
    return 0;
  }
}

void *tlcomb_store_bool(void **IP, void **Data, zval **arr, struct tl_tree **vars);

/****
 *
 * Data: [data] result => [data]
 * IP  :
 *
 ****/
void *tlcomb_store_type(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  //struct tl_type *t = *(IP ++);
  auto *e = *reinterpret_cast<tl_tree_type **>(--Data);
  assert (e);
  struct tl_type *t = e->type;
  assert (t);
#ifdef VLOG
  fprintf (stderr, "Storing type %s[%x]. *arr = %p\n", t->id, t->name, *arr);
#endif

  if (t->name == TYPE_NAME_BOOL) {
    DEC_REF(e);
    #ifdef VLOG
    fprintf(stderr, "Using hack to store bool\n");
    #endif
    return tlcomb_store_bool(IP, Data, arr, vars);
  }

  int n = -1;
  if (t->constructors_num > 1) {
    if (t->name == TYPE_NAME_MAYBE && typed_mode) {
      // hack for getting name of maybe constructor in typed mode
      const char *ctor_name;
      if (Z_TYPE_P(*arr) == IS_NULL) {
        ctor_name = "resultFalse";
      } else {
        ctor_name = "resultTrue";
      }
      n = get_constructor(t, ctor_name);
    } else {
      // IN PHP5 v == &dst => lifetime is limited by scope of this function
      zval *dst;
      VK_ZVAL_API_P v = get_field(*arr, "_", 0, &dst);
      if (v) {
        int sl;
        char *s = parse_zend_string(v, &sl);
        n = get_constructor(t, s);
        if (n < 0) {
          vkext_reset_error();
          vkext_error_format(VKEXT_ERROR_UNKNOWN_CONSTRUCTOR, "Unknown constructor %s for type %s", s, t->id);
          DEC_REF (e);
          if (Z_TYPE_P(*arr) == IS_OBJECT) {
            zval_ptr_dtor(v);
            efree(dst);
          }
          return 0;
        }
        if (Z_TYPE_P(*arr) == IS_OBJECT) {
          zval_ptr_dtor(v);
          efree(dst);
        }
      }
    }
  } else {
    n = 0;
  }
  if (n >= 0) {
#ifdef VLOG
    fprintf (stderr, "Storing constructor %s\n", t->constructors[n]->id);
#endif
    *(Data) = e;
    if (!(e->self.flags & FLAG_BARE) && strcmp(t->constructors[n]->id, "_")) {
      do_rpc_store_int(t->constructors[n]->name);
    }
    struct tl_tree **new_vars = get_var_space(vars, t->constructors[n]->var_num);
    if (!new_vars) {
      vkext_reset_error();
      vkext_error_format(VKEXT_ERROR_NOT_ENOUGH_MEMORY, "Not enough memory to store %s[%s]", t->id, t->constructors[n]->id);
      return 0;
    }
    if (TLUNI_START (t->constructors[n]->IP, Data + 1, arr, new_vars) != TLUNI_OK) {
      vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Fail to store %s[%s]", t->id, t->constructors[n]->id);
      DEC_REF (e);
      return 0;
    }
    DEC_REF (e);
    TLUNI_NEXT;
  } else {
#ifdef VLOG
    fprintf (stderr, "Storing any constructor\n");
#endif
    php_error_docref(NULL, E_WARNING,
                     "### DEPRECATED TL FEATURE ###: "
                     "The constructor name must be given if type has several ones. It's going to guess the constructor in runtime (storing phase) ! "
                     "Type: %s; "
                     "Function: %s",
                     t->id, tl_current_function_name);
    int k = do_rpc_store_get_pos();
    for (n = 0; n < t->constructors_num; n++) {
      if (!(e->self.flags & FLAG_BARE) && strcmp(t->constructors[n]->id, "_")) {
        do_rpc_store_int(t->constructors[n]->name);
      }
      *(Data) = e;
      struct tl_tree **new_vars = get_var_space(vars, t->constructors[n]->var_num);
      if (!new_vars) {
        vkext_reset_error();
        vkext_error_format(VKEXT_ERROR_NOT_ENOUGH_MEMORY, "Not enough memory to store %s", t->id);
        return 0;
      }
      void *r = TLUNI_START (t->constructors[n]->IP, Data + 1, arr, new_vars);
      if (r == TLUNI_OK) {
        DEC_REF (e);
        TLUNI_NEXT;
      }
      assert (do_rpc_store_set_pos(k));
    }
    vkext_reset_error();
    vkext_error_format(VKEXT_ERROR_UNKNOWN, "Can't store type %s: constructor not specified [WTF this shouldn't be compilable in TL]", t->id);
    DEC_REF (e);
    return 0;
  }
}

/****
 *
 * Data: [data] => [data]
 * IP  : id num
 *
 ***/
void *tlcomb_store_field(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  char *id = *reinterpret_cast<char **>(IP++);
  int num = (long)*(IP++);
  int type_magic = (long)*(IP++);
#ifdef VLOG
  fprintf (stderr, "store_field: id = %s, num = %d\n", id, num);
#endif

  zval *dst;
  // IN PHP5 v == &dst => lifetime is limited by scope of this function
  VK_ZVAL_API_P v = get_field(*arr, id, num, &dst);
#ifdef VLOG
  fprintf (stderr, "store_field: field %p\n", v);
#endif
  if (!v) {
    vkext_reset_error();
    vkext_error_format(VKEXT_ERROR_NO_FIELD, "Field %s not found", id);
    return 0;
  }
  if (type_magic != TYPE_NAME_MAYBE && VK_Z_API_TYPE(v) == IS_NULL) {
    php_error(E_WARNING, "Null found instead of field \"%s\". This will cause error in kPHP.\n", id);
  }
  *(++arr) = VK_ZVAL_API_TO_ZVALP(v);
  TLUNI_NEXT;
}


/****
 *
 * Data: [data] => [data]
 *
 ***/

void *tlcomb_store_field_end(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  --arr;
  TLUNI_NEXT;
}


/****
 *
 * Data: [data] => [data]
 * IP: id num
 *
 ***/

void *tlcomb_fetch_field_end(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  char *id = *reinterpret_cast<char **>(IP++);
  int num = (long)*(IP++);
  if (!*arr) {
    php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nField is NULL after fetching", tl_current_function_name);
    VK_ALLOC_INIT_ZVAL (*arr);
    array_init (*arr);
  }
  set_field((arr - 1), *arr, id, num);
  arr--;
  TLUNI_NEXT;
}


/****
 *
 * Data: [data] arity => [data]
 * IP  : newIP
 *
 ***/
void *tlcomb_store_array(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  void **newIP = static_cast<void **>(*(IP++));
  int multiplicity = var_nat_const_to_int(*(--Data));
#ifdef VLOG
  fprintf (stderr, "multiplicity %d. *arr = %p\n", multiplicity, *arr);
#endif
  DEC_REF (*Data);
  int i;
  for (i = 0; i < multiplicity; i++) {
    VK_ZVAL_API_P w = get_field(*arr, 0, i, NULL);
    #ifdef VLOG
    fprintf (stderr, "field = %p\n", w ? *w : 0);
    #endif
    if (!w) {
      vkext_reset_error();
      vkext_error_format(VKEXT_ERROR_INVALID_ARRAY_SIZE, "No element #%d in array", i);
      return 0;
    }
    *(++arr) = VK_ZVAL_API_TO_ZVALP(w);
    if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
      vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Can't parse element #%d in array", i);
      --arr;
      return 0;
    }
    --arr;
  }
  TLUNI_NEXT;
}

/****
 *
 * Data: [data] arity => [data]
 * IP  : newIP
 *
 ***/
void *tlcomb_fetch_array(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int num = (long)*(IP++);
  int is_flat = (long)*(IP++);
  const char *owner_name = (char *)*(IP++);
  //fprintf(stderr, "~~~~~~~~~ owner name = %s flat = %d\n", owner_name, is_flat);
  void **newIP = static_cast<void **>(*(IP++));
  int multiplicity = var_nat_const_to_int(*(--Data));
#ifdef VLOG
  fprintf (stderr, "multiplicity %d\n", multiplicity);
#endif
  DEC_REF (*Data);
  VK_ALLOC_INIT_ZVAL (*arr);
  array_init (*arr);
  for (int i = 0; i < multiplicity; i++) {
    *(++arr) = 0;
    if (!is_flat) {
      if (typed_mode) {
        static char class_name[PHP_CLASS_NAME_BUFFER_LENGTH];
        static char php_prefix[PHP_PREFIX_BUFFER_LENGTH];
        const size_t buf_size = 30;
        static char buf[buf_size];
        snprintf(buf, buf_size, "_arg%d_item", num);
        get_php_prefix(php_prefix, owner_name, TYPES);
        check_buffer_overflow(make_tl_class_name(class_name, php_prefix, owner_name, buf, '_'));
        *arr = create_php_instance(class_name);
      } else {
        VK_ALLOC_INIT_ZVAL (*arr);
        array_init (*arr);
      }
    }
    if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
      if (!is_flat) {
        zval_ptr_dtor(VK_ZVALP_TO_API_ZVALP(*arr));
        *arr = 0;
      }
      --arr;
      return 0;
    }
    if (!*arr) {
      php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nElement of tl array is NULL after fetching", tl_current_function_name);
      VK_ALLOC_INIT_ZVAL (*arr);
      array_init (*arr);
    }
    vk_add_index_zval_nod(*(arr - 1), i, *arr);
    --arr;
  }
  TLUNI_NEXT;
}

static bool is_int32_overflow(int64_t v) {
  const auto v32 = static_cast<int32_t>(v);
  return vk::none_of_equal(v, int64_t{v32}, int64_t{static_cast<uint32_t>(v32)});
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_store_int(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int64_t v = parse_zend_long(Z_PP_TO_ZAPI_P(arr));
  const auto &v32 = static_cast<int32_t>(v);
#ifdef VLOG
  fprintf (stderr, "Got int %d (0x%08x)\n", v32, v32);
#endif
  if (fail_rpc_on_int32_overflow && is_int32_overflow(v)) {
    vkext_reset_error();
    vkext_error_format(VKEXT_ERROR_WRONG_VALUE, "Got int32 overflow on storing TL function %s: the value '%" PRIi64 "' will be casted to '%d'",
                       tl_current_function_name, v, v32);
    return nullptr;
  }
  do_rpc_store_int(v32);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_fetch_int(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int a = tl_parse_int();
#ifdef VLOG
  fprintf (stderr, "!!!%d (0x%08x). error %s\n", a, a, tl_parse_error () ? tl_parse_error () : "none");
#endif
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  ZVAL_LONG (*arr, a);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_fetch_long(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  long long a = tl_parse_long();
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  ZVAL_LONG (*arr, a);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_fetch_double(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  double a = tl_parse_double();
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  ZVAL_DOUBLE (*arr, a);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_fetch_float(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  float a = tl_parse_float();
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  ZVAL_DOUBLE (*arr, a);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_fetch_false(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  if (tl_parse_error()) {
    return 0;
  }
#ifdef VLOG
    fprintf (stderr, "fetch false\n");
#endif
  VK_ALLOC_INIT_ZVAL (*arr);
  ZVAL_FALSE (*arr);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_fetch_true(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  if (tl_parse_error()) {
    return 0;
  }
#ifdef VLOG
    fprintf (stderr, "fetch true\n");
#endif
  VK_ALLOC_INIT_ZVAL (*arr);
  ZVAL_TRUE (*arr);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP: newIP
 *
 *****/
void *tlcomb_fetch_vector(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int n = tl_parse_int();
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  array_init (*arr);
  void **newIP = static_cast<void **>(*(IP++));

#ifdef VLOG
  fprintf (stderr, "multiplicity %d\n", n);
#endif
  int i;
  for (i = 0; i < n; i++) {
    *(++arr) = 0;
    if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
      --arr;
      return 0;
    }
    if (!*arr) {
      php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nElement of Vector is NULL after fetching", tl_current_function_name);
      VK_ALLOC_INIT_ZVAL (*arr);
      array_init (*arr);
    }
    vk_add_index_zval_nod(*(arr - 1), i, *arr);
    --arr;
  }
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP: newIP
 *
 *****/
void *tlcomb_fetch_dictionary(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int n = tl_parse_int();
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  array_init (*arr);
  void **newIP = static_cast<void **>(*(IP++));

#ifdef VLOG
  fprintf (stderr, "multiplicity %d\n", n);
#endif
  int i;
  for (i = 0; i < n; i++) {
    *(++arr) = 0;
    char *s = 0;
    tl_parse_string(&s);
    if (tl_parse_error()) {
      return 0;
    }

    if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
      free(s);
      --arr;
      return 0;
    }
    if (!*arr) {
      php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nElement of Dictionary is NULL after fetching", tl_current_function_name);
      VK_ALLOC_INIT_ZVAL (*arr);
      array_init (*arr);
    }
    array_set_field(arr - 1, *arr, s, 0);
    --arr;
    free(s);
  }
  TLUNI_NEXT;
}

void *tlcomb_fetch_int_key_dictionary(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int n = tl_parse_int();
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  array_init (*arr);
  void **newIP = static_cast<void **>(*(IP++));

#ifdef VLOG
  fprintf (stderr, "multiplicity %d\n", n);
#endif
  int i;
  for (i = 0; i < n; i++) {
    *(++arr) = 0;
    int key = tl_parse_int();
    if (tl_parse_error()) {
      return 0;
    }

    if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
      --arr;
      return 0;
    }
    if (!*arr) {
      php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nElement of IntKeyDictionary is NULL after fetching", tl_current_function_name);
      VK_ALLOC_INIT_ZVAL (*arr);
      array_init (*arr);
    }
    array_set_field(arr - 1, *arr, NULL, key);
    --arr;
  }
  TLUNI_NEXT;
}

void *tlcomb_fetch_long_key_dictionary(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int n = tl_parse_int();
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  array_init (*arr);
  void **newIP = static_cast<void **>(*(IP++));

#ifdef VLOG
  fprintf (stderr, "multiplicity %d\n", n);
#endif
  int i;
  for (i = 0; i < n; i++) {
    *(++arr) = 0;
    long long key = tl_parse_long();
    if (tl_parse_error()) {
      return 0;
    }

    if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
      --arr;
      return 0;
    }
    if (!*arr) {
      php_error_docref(NULL, E_WARNING, "In fetching of the function: %s\nElement of LongKeyDictionary i NULL after fetching", tl_current_function_name);
      VK_ALLOC_INIT_ZVAL (*arr);
      array_init (*arr);
    }
    array_set_field(arr - 1, *arr, NULL, key);
    --arr;
  }
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP: newIP
 *
 *****/
void *tlcomb_fetch_maybe(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  void **newIP = static_cast<void **>(*(IP++));
  if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
    --arr;
    return 0;
  }
  TLUNI_NEXT;
}

void *tlcomb_fetch_maybe_false(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL (*arr);
  if (typed_mode) {
    ZVAL_NULL (*arr);
  } else {
    ZVAL_FALSE (*arr);
  };
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_fetch_string(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  char *a;
  int l = tl_eparse_string(&a);
  if (tl_parse_error()) {
    return 0;
  }
  if (!*arr) {
    VK_ALLOC_INIT_ZVAL(*arr);
  }
  VK_ZVAL_STRINGL_NOD(*arr, a, l);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP: var_num
 *
 *****/
void *tlcomb_store_var_num(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int var_num = (long)*(IP++);
  int a = parse_zend_long(Z_PP_TO_ZAPI_P(arr));
  if (vars[var_num]) {
    DEC_REF (vars[var_num]);
  }
  vars[var_num] = reinterpret_cast<tl_tree *>(int_to_var_nat_const(a));
  do_rpc_store_int(a);
  TLUNI_NEXT;
}


/*****
 *
 * Data [data] => [data]
 * IP: var_num
 *
 *****/
void *tlcomb_fetch_var_num(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int x = tl_parse_int();
#ifdef VLOG
  fprintf (stderr, "%d\n", x);
#endif
  if (tl_parse_error()) {
    return 0;
  }
  VK_ALLOC_INIT_ZVAL(*arr);
  ZVAL_LONG (*arr, x);
  int var_num = (long)*(IP++);
  if (vars[var_num]) {
    DEC_REF (vars[var_num]);
  }
  vars[var_num] = reinterpret_cast<tl_tree *>(int_to_var_nat_const(x));

  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP: var_num
 *
 *****/
void *tlcomb_fetch_check_var_num(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int x = tl_parse_int();
#ifdef VLOG
  fprintf (stderr, "%d\n", x);
#endif
  if (tl_parse_error()) {
    return 0;
  }
  int var_num = (long)*(IP++);
  if (x != var_nat_const_to_int(vars[var_num])) {
    return 0;
  }
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP: var_num flags
 *
 *****/
void *tlcomb_store_var_type(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int var_num = (long)*(IP++);
  int flags = (long)*(IP++);
  int l;
  char *a = parse_zend_string(Z_PP_TO_ZAPI_P(arr), &l);
  if (!a) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_INVALID_CONSTRUCTOR, "Can't parse string");
    return 0;
  }
  if (vars[var_num]) {
    DEC_REF (vars[var_num]);
    vars[var_num] = 0;
  }
  struct tl_type *t = tl_type_get_by_id(a);
  if (!t) {
    vkext_reset_error();
    vkext_error_format(VKEXT_ERROR_UNKNOWN_TYPE, "Unknown type \"%s\"", a);
    return 0;
  }

  auto *x = reinterpret_cast<tl_tree_type *>(zzemalloc(sizeof(tl_tree_type)));
  dynamic_tree_nodes++;
  total_tree_nodes_existed++;
  x->self.ref_cnt = 1;
  total_ref_cnt++;
  x->self.flags = flags;
  x->self.methods = &tl_type_methods;
  x->children_num = 0;
  x->type = t;
  x->children = 0;
  vars[var_num] = reinterpret_cast<tl_tree *>(x);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_store_long(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  long long a = parse_zend_long(Z_PP_TO_ZAPI_P(arr));
  do_rpc_store_long(a);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_store_double(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  double a = parse_zend_double(Z_PP_TO_ZAPI_P(arr));
  do_rpc_store_double(a);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_store_float(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  float a = (float) parse_zend_double(Z_PP_TO_ZAPI_P(arr));
  do_rpc_store_float(a);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP: newIP
 *
 *****/
void *tlcomb_store_maybe(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  void **newIP = static_cast<void **>(*(IP++));
  if (Z_TYPE_P(*arr) == IS_ARRAY && vk_zend_hash_exists(Z_ARRVAL_P(*arr), "_", 1)) {
    // hack for untyped storing
    VK_ZVAL_API_P w = get_field(*arr, "result", 2, NULL);
    *(++arr) = VK_ZVAL_API_TO_ZVALP(w);
  }
  if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
    --arr;
    return 0;
  }
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_store_bool(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int a = parse_zend_bool(Z_PP_TO_ZAPI_P(arr));
  do_rpc_store_int(a ? NAME_BOOL_TRUE : NAME_BOOL_FALSE);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_store_vector(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  void **newIP = static_cast<void **>(*(IP++));
  if (Z_TYPE_P (*arr) != IS_ARRAY) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_WRONG_TYPE, "Can't store not array as vector");
    return 0;
  }

  int multiplicity = get_array_size(arr);
  do_rpc_store_int(multiplicity);

  int i;
  for (i = 0; i < multiplicity; i++) {
    VK_ZVAL_API_P w = get_field(*arr, 0, i, NULL);
    #ifdef VLOG
    fprintf (stderr, "field = %p\n", w ? *w : 0);
    #endif
    if (!w) {
      vkext_reset_error();
      vkext_error_format(VKEXT_ERROR_INVALID_ARRAY_SIZE, "No element #%d in array", i);
      return 0;
    }
    *(++arr) = VK_ZVAL_API_TO_ZVALP(w);
    if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
      --arr;
      vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Can't store element #%d of vector", i);
      return 0;
    }
    --arr;
  }
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_store_dictionary(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  void **newIP = static_cast<void **>(*(IP++));
  if (Z_TYPE_P (*arr) != IS_ARRAY) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_WRONG_TYPE, "Can't store not array as dictionary");
    return 0;
  }

  int multiplicity = get_array_size(arr);
  do_rpc_store_int(multiplicity);

  int i = 0;
  VK_ZVAL_API_P zkey = 0;
  NEW_INIT_Z_STR_P(key);
  zend_ulong index;

  VK_ZEND_HASHES_FOREACH_KEY_VAL(Z_ARRVAL_P(*arr), index, key, zkey) {
    if (VK_ZSTR_P_NON_EMPTY(key)) {
      do_rpc_store_string(key->val, VK_HASH_KEY_LEN(key->len));
    } else {
      const size_t buf_size = 30;
      static char buf[buf_size];
      snprintf(buf, buf_size, "%" PRIu64, static_cast<uint64_t>(index));
      do_rpc_store_string(buf, strlen(buf));
    }

    *(++arr) = VK_ZVAL_API_TO_ZVALP(zkey);
    if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
      vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Fail while storing key %s in dictionary", key->val);
      --arr;
      return 0;
    }
    --arr;
    i++;
  } VK_ZEND_HASH_FOREACH_END();

  if (i != multiplicity) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_UNKNOWN, "Fail while storing dictionary: number of keys is not equal to size");
    return 0;
  }
  TLUNI_NEXT;
}

void *tlcomb_store_int_key_dictionary(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  void **newIP = static_cast<void **>(*(IP++));
  if (Z_TYPE_P (*arr) != IS_ARRAY) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_WRONG_TYPE, "Can't store not array as dictionary");
    return 0;
  }

  int multiplicity = get_array_size(arr);
  do_rpc_store_int(multiplicity);

  int i = 0;
  VK_ZVAL_API_P zkey = 0;
  NEW_INIT_Z_STR_P(key);
  zend_ulong index;
  VK_ZEND_HASHES_FOREACH_KEY_VAL(Z_ARRVAL_P(*arr), index, key, zkey) {
        do_rpc_store_int((int)index);

        *(++arr) = VK_ZVAL_API_TO_ZVALP(zkey);
        if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
          vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Fail while storing key %s in dictionary", key->val);
          --arr;
          return 0;
        }
        --arr;
        i++;
      } VK_ZEND_HASH_FOREACH_END();

  if (i != multiplicity) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_UNKNOWN, "Fail while storing dictionary: number of keys is not equal to size");
    return 0;
  }
  TLUNI_NEXT;
}

void *tlcomb_store_long_key_dictionary(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  void **newIP = static_cast<void **>(*(IP++));
  if (Z_TYPE_P (*arr) != IS_ARRAY) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_WRONG_TYPE, "Can't store not array as dictionary");
    return 0;
  }

  int multiplicity = get_array_size(arr);
  do_rpc_store_int(multiplicity);

  int i = 0;
  VK_ZVAL_API_P zkey = 0;
  NEW_INIT_Z_STR_P(key);
  zend_ulong index;

  VK_ZEND_HASHES_FOREACH_KEY_VAL(Z_ARRVAL_P(*arr), index, key, zkey) {
        do_rpc_store_long((long long)index);

        *(++arr) = VK_ZVAL_API_TO_ZVALP(zkey);
        if (TLUNI_START (newIP, Data, arr, vars) != TLUNI_OK) {
          vkext_error_format(VKEXT_ERROR_CONSTRUCTION, "Fail while storing key %s in dictionary", key->val);
          --arr;
          return 0;
        }
        --arr;
        i++;
      } VK_ZEND_HASH_FOREACH_END();

  if (i != multiplicity) {
    vkext_reset_error();
    vkext_error(VKEXT_ERROR_UNKNOWN, "Fail while storing dictionary: number of keys is not equal to size");
    return 0;
  }
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP:
 *
 *****/
void *tlcomb_store_string(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int len;
  char *s = parse_zend_string(Z_PP_TO_ZAPI_P(arr), &len);
  do_rpc_store_string(s, len);
  TLUNI_NEXT;
}

/*****
 *
 * Data [data] => [data]
 * IP: var_num bit_num shift
 *
 *****/
void *tlcomb_check_bit(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  long n = (long)*(IP++);
  long b = (long)*(IP++);
  long o = (long)*(IP++);
#ifdef VLOG
  fprintf (stderr, "check_bit. var_num = %ld, bit_num = %ld, offset = %ld\n", n, b, o);
#endif
  long x = 0;
  if (vars[n] != NULL) {
    x = var_nat_const_to_int(vars[n]);
  }
#ifdef VLOG
  fprintf (stderr, "check_bit. var_num = %ld, bit_num = %ld, offset = %ld, var_value = %ld\n", n, b, o, x);
#endif
  if (!(x & (1 << b))) {
    IP += o;
  }
  TLUNI_NEXT;
}

/*** }}} ***/

/**** Uniformize code {{{ ****/

/*****
 *
 * Data: [Data] res => [Data] childn ... child2 child1
 * IP: type
 *
 *****/
void *tluni_check_type(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  auto *res = *reinterpret_cast<tl_tree_type **>(--Data);
//  fprintf (stderr, "res = %p, res->type = %p, *IP = %p, *IP->id = %s\n", res, res->type, *IP, ((struct tl_type *)*IP)->id);

  if (TL_TREE_METHODS (res)->type(reinterpret_cast<tl_tree *>(res)) != NODE_TYPE_TYPE) {
    return 0;
  }
  if (res->type != *(IP++)) {
    return 0;
  }
//  if ((res->self.flags & FLAGS_MASK) != (long)*(IP ++)) { return 0; }

  int i;
  for (i = res->children_num - 1; i >= 0; i--) {
    *(Data++) = res->children[i];
  }
  TLUNI_NEXT;
}

/*****
 *
 * Data: [Data] res => [Data]
 * IP: const
 *
 *****/
void *tluni_check_nat_const(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  if (*(IP++) != *(--Data)) {
    return 0;
  }
  TLUNI_NEXT;
}

/*****
 *
 * Data: [Data] res => [Data] childn ... child2 child1 multiplicity
 * IP: array
 *
 *****/
void *tluni_check_array(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  auto *res = *reinterpret_cast<tl_tree_array **>(--Data);
  if (TL_TREE_METHODS (res)->type(reinterpret_cast<tl_tree *>(res)) != NODE_TYPE_ARRAY) {
    return 0;
  }
  if (res->args_num != (long)*(IP++)) {
    return 0;
  }
  int i;
  for (i = res->args_num - 1; i >= 0; i--) {
    *(Data++) = res->args[i];
  }
  *(Data++) = res->multiplicity;
  TLUNI_NEXT;
}

/*****
 *
 * Data [Data] child => [Data] type
 * IP arg_name
 *
 *****/
void *tluni_check_arg(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  arg *res = *reinterpret_cast<arg **>(--Data);
  const char *id = *reinterpret_cast<char **>(IP++);
  if (!id) {
    if (res->id) {
      return 0;
    }
  } else {
    if (!res->id) {
      return 0;
    }
    if (strcmp(id, res->id)) {
      return 0;
    }
  }
  *(Data++) = res->type;
  TLUNI_NEXT;
}

/*****
 *
 * Data [Data] value => [Data]
 * IP var_num add_value
 *
 *****/
void *tluni_set_nat_var(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  long p = (long)*(IP++);
  if (vars[p]) {
    DEC_REF (vars[p]);
  }
  vars[p] = 0;
  void *a = *(--Data);
  void *x = int_to_var_nat_const(var_nat_const_to_int(a) + (long)*(IP++));
  //DEC_REF (a);
  //void *x = *(--Data) + 2 * (long)*(IP ++);
  //fprintf (stderr, "c = %lld\n", (long long)var_nat_const_to_int (x));
  //if (!TL_IS_NAT_VAR (x)) {  return 0; }
  if (var_nat_const_to_int(x) < 0) {
    DEC_REF (x);
    return 0;
  }
  vars[p] = reinterpret_cast<tl_tree *>(x);
  TLUNI_NEXT;
}

/*****
 *
 * Data [Data] value => [Data]
 * IP var_num
 *
 *****/
void *tluni_set_type_var(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  long p = (long)*(IP++);
  if (vars[p]) {
    DEC_REF (vars[p]);
  }
  vars[p] = 0;
//  fprintf (stderr, "p = %ld, var = %p, var->type = %s\n", p, vars[p], ((struct tl_tree_type *)vars[p])->type->id);
  vars[p] = *reinterpret_cast<tl_tree **>(--Data);
  if (TL_IS_NAT_VAR (vars[p])) {
    return 0;
  }
  INC_REF (vars[p]);
  TLUNI_NEXT;
}


/*****
 *
 * Data [Data] value => [Data]
 * IP var_num add_value
 *
 *****/
void *tluni_check_nat_var(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  long p = (long)*(IP++);
  //void *x = *(--Data) + 2 * (long)*(IP ++);
  void *x = int_to_var_nat_const(var_nat_const_to_int(--Data) + (long)*(IP++));
  if (vars[p] != x) {
    DEC_REF (x);
    return 0;
  }
  DEC_REF (x);
  TLUNI_NEXT;
}


/*****
 *
 * Data [Data] value => [Data]
 * IP var_num
 *
 *****/
void *tluni_check_type_var(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  tl_tree *x = *reinterpret_cast<tl_tree **>(--Data);
  tl_tree *y = vars[(long)*(IP++)];
  if (!TL_TREE_METHODS (y)->eq(y, x)) {
    return 0;
  }
  TLUNI_NEXT;
}


/*****
 *
 * Data [Data] multiplicity type1 ... typen => [Data] array
 * IP flags args_num idn ... id1
 *
 *****/
void *tlsub_create_array(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  tl_tree_array *x = reinterpret_cast<tl_tree_array *>(zzemalloc(sizeof(*x)));
  dynamic_tree_nodes++;
  total_tree_nodes_existed++;
  x->self.ref_cnt = 1;
  x->self.flags = (long)*(IP++);
  x->self.methods = &tl_array_methods;
  x->args_num = (long)*(IP++);
  int i;
  x->args = reinterpret_cast<arg **>(zzemalloc(sizeof(void *) * x->args_num));
  for (i = x->args_num - 1; i >= 0; i--) {
    x->args[i] = reinterpret_cast<arg *>(zzemalloc(sizeof(*x->args[i])));
    x->args[i]->id = *reinterpret_cast<char **>(IP++);
    x->args[i]->type = *reinterpret_cast<tl_tree **>(--Data);
//    TL_TREE_METHODS (x->args[i]->type)->inc_ref (x->args[i]->type);
  }
  x->multiplicity = *reinterpret_cast<tl_tree **>(--Data);
#ifdef VLOG
  fprintf (stderr, "Create array\n");
#endif
//  TL_TREE_METHODS (x->multiplicity)->inc_ref (x->multiplicity);
  *(Data++) = x;
  TLUNI_NEXT;
}


/*****
 *
 * Data [Data] type1 ... typen  => [Data] type
 * IP flags type_ptr
 *
 *****/
void *tlsub_create_type(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  auto *x = reinterpret_cast<tl_tree_type *>(zzemalloc(sizeof(tl_tree_type)));
  dynamic_tree_nodes++;
  total_tree_nodes_existed++;
  x->self.ref_cnt = 1;
  total_ref_cnt++;
  x->self.flags = (long)*(IP++);
  x->self.methods = &tl_type_methods;
  x->type = *reinterpret_cast<tl_type **>(IP++);
#ifdef VLOG
  fprintf (stderr, "Create type %s. flags = %d\n", x->type->id, x->self.flags);
#endif
  x->children_num = x->type->arity;
  x->children = reinterpret_cast<tl_tree **>(zzemalloc(sizeof(void *) * x->children_num));
  int i;
  for (i = x->children_num - 1; i >= 0; i--) {
    x->children[i] = *reinterpret_cast<tl_tree **>(--Data);
//    TL_TREE_METHODS (x->children[i])->inc_ref (x->children[i]);
  }
  *(Data++) = x;
//  fprintf (stderr, "create type %s\n", x->type->id);
  TLUNI_NEXT;
}

void *tlsub_ret_ok(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  return TLUNI_OK;
}

void *tlsub_ret(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  return *(--Data);
}

void *tlsub_ret_zero(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  return 0;
}

/*void *tlsub_push_const (void **IP, void **Data, zval **arr, struct tl_tree **vars) {
 * tl_debug(__FUNCTION__, -1);
  *(Data ++) = *(IP ++);
  TLUNI_NEXT;
}*/

void *tlsub_push_type_var(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  *(Data++) = vars[(long)*(IP++)];
#ifdef VLOG
  fprintf (stderr, "Push type var\n");
#endif
  INC_REF (*(Data - 1));
  TLUNI_NEXT;
}

void *tlsub_push_nat_var(void **IP, void **Data, zval **arr, struct tl_tree **vars) {
  tl_debug(__FUNCTION__, -1);
  int p = (long)*(IP++);
//  *(Data ++) = ((void *)vars[p]) + 2 * (long) *(IP ++);
//  fprintf (stderr, "vars[p] = %p\n", vars[p]);
  *(Data++) = int_to_var_nat_const(var_nat_const_to_int(vars[p]) + (long)*(IP++));
#ifdef VLOG
  fprintf (stderr, "Push nat var. Data = %lld\n", var_nat_const_to_int (*(Data - 1)));
#endif
  TLUNI_NEXT;
}
/* }}} */

/**** Default tree methods {{{ ****/


int tl_tree_eq_type(struct tl_tree *_x, struct tl_tree *_y) {
  if (TL_TREE_METHODS (_y)->type((tl_tree *)_y) != NODE_TYPE_TYPE) {
    return 0;
  }
  auto *x = (tl_tree_type *)_x;
  auto *y = (tl_tree_type *)_y;
  if ((x->self.flags & FLAGS_MASK) != (y->self.flags & FLAGS_MASK)) {
    return 0;
  }
  if (x->type != y->type) {
    return 0;
  }
  int i;
  for (i = 0; i < x->children_num; i++) {
    if (!TL_TREE_METHODS (x->children[i])->eq(x->children[i], y->children[i])) {
      return 0;
    }
  }
  return 1;
}

int tl_tree_eq_array(struct tl_tree *_x, struct tl_tree *_y) {
  if (TL_TREE_METHODS (_y)->type((tl_tree *)_y) != NODE_TYPE_ARRAY) {
    return 0;
  }
  auto *x = (tl_tree_array *)_x;
  auto *y = (tl_tree_array *)_y;
  if (x->self.flags != y->self.flags) {
    return 0;
  }
  if (x->args_num != y->args_num) {
    return 0;
  }
  int i;
  for (i = 0; i < x->args_num; i++) {
    if (!x->args[i]->id) {
      if (y->args[i]->id) {
        return 0;
      }
    } else {
      if (!y->args[i]->id) {
        return 0;
      }
      if (strcmp(x->args[i]->id, y->args[i]->id)) {
        return 0;
      }
    }
    if (!TL_TREE_METHODS (x->args[i]->type)->eq(x->args[i]->type, y->args[i]->type)) {
      return 0;
    }
  }
  return 1;
}

int tl_tree_eq_nat_const(struct tl_tree *x, struct tl_tree *y) {
  //return (x == y);
  return var_nat_const_to_int(x) == var_nat_const_to_int(y);
}

void tl_tree_inc_ref_type(struct tl_tree *x) {
  total_ref_cnt++;
  x->ref_cnt++;
  //fprintf (stderr, "Inc_ref: type %s\n", ((struct tl_tree_type *)x)->type->id);
}

void tl_tree_inc_ref_array(struct tl_tree *x) {
  total_ref_cnt++;
  //fprintf (stderr, "Inc_ref: array\n");
  x->ref_cnt++;
}

void tl_tree_inc_ref_nat_const(struct tl_tree *x) {
}

void tl_tree_inc_ref_nat_const_full(struct tl_tree *x) {
  total_ref_cnt++;
  x->ref_cnt++;
#ifdef VLOG
  fprintf (stderr, "Inc_ref: nat_const_full. value = %lld, new refcnt = %d\n", ((struct tl_tree_nat_const*)x)->value, x->ref_cnt);
#endif
}

void tl_tree_dec_ref_type(struct tl_tree *_x) {
  total_ref_cnt--;
  auto *x = (tl_tree_type *)_x;
#ifdef VLOG
  fprintf (stderr, "Dec_ref: type %s. Self ref_cnt = %d, children_num = %d\n", x->type->id, x->self.ref_cnt, x->children_num);
#endif
  x->self.ref_cnt--;
  if (x->self.ref_cnt > 0) {
    return;
  }
  dynamic_tree_nodes--;
  int i;
  for (i = 0; i < x->children_num; i++) {
    DEC_REF (x->children[i]);
  }
  zzefree(x->children, sizeof(void *) * x->children_num);
  zzefree(x, sizeof(*x));
}

void tl_tree_dec_ref_array(struct tl_tree *_x) {
  total_ref_cnt--;
#ifdef VLOG
  fprintf (stderr, "Dec_ref: array\n");
#endif
  auto *x = (tl_tree_array *)_x;
  x->self.ref_cnt--;
  if (x->self.ref_cnt > 0) {
    return;
  }
  dynamic_tree_nodes--;
  int i;
  DEC_REF (x->multiplicity);
  for (i = 0; i < x->args_num; i--) {
    DEC_REF (x->args[i]->type);
    zzefree(x->args[i], sizeof(*x->args[i]));
  }
  zzefree(x->args, sizeof(void *) * x->args_num);
  zzefree(x, sizeof(*x));
}

void tl_tree_dec_ref_ptype(struct tl_tree *_x) {
  total_ref_cnt--;
  auto *x = (tl_tree_type *)_x;
#ifdef VLOG
  fprintf (stderr, "Dec_ref: persistent type %s. Self ref_cnt = %d, children_num = %d\n", x->type->id, x->self.ref_cnt, x->children_num);
#endif
  x->self.ref_cnt--;
  if (x->self.ref_cnt > 0) {
    return;
  }
  persistent_tree_nodes--;
  int i;
  for (i = 0; i < x->children_num; i++) {
    DEC_REF (x->children[i]);
  }
  zzfree(x->children, sizeof(void *) * x->children_num);
  ADD_PFREE (sizeof(void *) * x->children_num);
  zzfree(x, sizeof(*x));
  ADD_PFREE (sizeof(*x));
}

void tl_tree_dec_ref_pvar_type(struct tl_tree *_x) {
  total_ref_cnt--;
  auto *x = (tl_tree_var_type *)_x;
#ifdef VLOG
  fprintf (stderr, "Dec_ref: persistent var_type\n");
#endif
  x->self.ref_cnt--;
  if (x->self.ref_cnt > 0) {
    return;
  }
  persistent_tree_nodes--;
  zzfree(x, sizeof(*x));
  ADD_PFREE (sizeof(*x));
}

void tl_tree_dec_ref_pvar_num(struct tl_tree *_x) {
  total_ref_cnt--;
  auto *x = (tl_tree_var_num *)_x;
#ifdef VLOG
  fprintf (stderr, "Dec_ref: persistent var_num\n");
#endif
  x->self.ref_cnt--;
  if (x->self.ref_cnt > 0) {
    return;
  }
  persistent_tree_nodes--;
  zzfree(x, sizeof(*x));
  ADD_PFREE (sizeof(*x));
}

void tl_tree_dec_ref_parray(struct tl_tree *_x) {
  total_ref_cnt--;
#ifdef VLOG
  fprintf (stderr, "Dec_ref: persistent array\n");
#endif
  auto *x = (tl_tree_array *)_x;
  x->self.ref_cnt--;
  if (x->self.ref_cnt > 0) {
    return;
  }
  persistent_tree_nodes--;
  int i;
  DEC_REF (x->multiplicity);
  for (i = 0; i < x->args_num; i++) {
    DEC_REF (x->args[i]->type);
    if (x->args[i]->id) {
      ADD_PFREE (strlen(x->args[i]->id));
      zzstrfree(x->args[i]->id);
//      free (x->args[i]->id);
    }
    zzfree(x->args[i], sizeof(*x->args[i]));
    ADD_PFREE (sizeof(*(x->args[i])));
  }
  zzfree(x->args, sizeof(void *) * x->args_num);
  ADD_PFREE (sizeof(void *) * x->args_num);
  zzfree(x, sizeof(*x));
  ADD_PFREE (sizeof(*x));
}

void tl_tree_dec_ref_nat_const(struct tl_tree *_x) {
}

void tl_tree_dec_ref_nat_const_full(struct tl_tree *_x) {
  total_ref_cnt--;
  _x->ref_cnt--;
#ifdef VLOG
  fprintf (stderr, "Dec_ref: nat_const_full at %p. value = %lld, new ref_cnt = %d\n", _x, ((struct tl_tree_nat_const*)_x)->value, _x->ref_cnt);
#endif
  if (_x->ref_cnt > 0) {
    return;
  }
  dynamic_tree_nodes--;
  zzefree(_x, sizeof(struct tl_tree_nat_const));
}

void tl_tree_dec_ref_pnat_const_full(struct tl_tree *_x) {
  _x->ref_cnt--;
  total_ref_cnt--;
#ifdef VLOG

  fprintf (stderr, "Dec_ref: pnat_const_full. value = %lld, new ref_cnt = %d\n", ((struct tl_tree_nat_const*)_x)->value, _x->ref_cnt);
#endif
  if (_x->ref_cnt > 0) {
    return;
  }
  persistent_tree_nodes--;
  zzfree(_x, sizeof(struct tl_tree_nat_const));
  ADD_FREE (sizeof(struct tl_tree_nat_const));
}

int tl_tree_type_type(struct tl_tree *x) {
  return NODE_TYPE_TYPE;
}

int tl_tree_type_array(struct tl_tree *x) {
  return NODE_TYPE_ARRAY;
}

int tl_tree_type_nat_const(struct tl_tree *x) {
  return NODE_TYPE_NAT_CONST;
}

int tl_tree_type_var_num(struct tl_tree *x) {
  return NODE_TYPE_VAR_NUM;
}

int tl_tree_type_var_type(struct tl_tree *x) {
  return NODE_TYPE_VAR_TYPE;
}

struct tl_tree_methods tl_var_num_methods = {
  .type = tl_tree_type_var_num,
  .eq = 0,
  .inc_ref = 0,
  .dec_ref = 0,
};

struct tl_tree_methods tl_var_type_methods = {
  .type = tl_tree_type_var_type,
  .eq = 0,
  .inc_ref = 0,
  .dec_ref = 0,
};

struct tl_tree_methods tl_type_methods = {
  .type = tl_tree_type_type,
  .eq = tl_tree_eq_type,
  .inc_ref = tl_tree_inc_ref_type,
  .dec_ref = tl_tree_dec_ref_type,
};

struct tl_tree_methods tl_nat_const_methods = {
  .type = tl_tree_type_nat_const,
  .eq = tl_tree_eq_nat_const,
  .inc_ref = tl_tree_inc_ref_nat_const,
  .dec_ref = tl_tree_dec_ref_nat_const,
};

struct tl_tree_methods tl_array_methods = {
  .type = tl_tree_type_array,
  .eq = tl_tree_eq_array,
  .inc_ref = tl_tree_inc_ref_array,
  .dec_ref = tl_tree_dec_ref_array,
};

struct tl_tree_methods tl_ptype_methods = {
  .type = tl_tree_type_type,
  .eq = tl_tree_eq_type,
  .inc_ref = tl_tree_inc_ref_type,
  .dec_ref = tl_tree_dec_ref_ptype,
};

struct tl_tree_methods tl_parray_methods = {
  .type = tl_tree_type_array,
  .eq = tl_tree_eq_array,
  .inc_ref = tl_tree_inc_ref_array,
  .dec_ref = tl_tree_dec_ref_parray,
};

struct tl_tree_methods tl_pvar_num_methods = {
  .type = tl_tree_type_var_num,
  .eq = 0,
  .inc_ref = 0,
  .dec_ref = tl_tree_dec_ref_pvar_num,
};

struct tl_tree_methods tl_pvar_type_methods = {
  .type = tl_tree_type_var_type,
  .eq = 0,
  .inc_ref = 0,
  .dec_ref = tl_tree_dec_ref_pvar_type,
};

struct tl_tree_methods tl_nat_const_full_methods = {
  .type = tl_tree_type_nat_const,
  .eq = tl_tree_eq_nat_const,
  .inc_ref = tl_tree_inc_ref_nat_const_full,
  .dec_ref = tl_tree_dec_ref_nat_const_full,
};

struct tl_tree_methods tl_pnat_const_full_methods = {
  .type = tl_tree_type_nat_const,
  .eq = tl_tree_eq_nat_const,
  .inc_ref = tl_tree_inc_ref_nat_const_full,
  .dec_ref = tl_tree_dec_ref_pnat_const_full,
};

/* }}} */

/* gen IP {{{ */
void **IP_dup(void **x, int l) {
  void **r = static_cast<void **>(zzmalloc(sizeof(void *) * l));
  ADD_PMALLOC (sizeof(void *) * l);
  memcpy(r, x, sizeof(void *) * l);
  return r;
}

int gen_uni(struct tl_tree *t, void **IP, int max_size, int *vars);

int gen_uni_arg(struct arg *arg, void **IP, int max_size, int *vars) {
  if (max_size <= 10) {
    return -1;
  }
  int l = 0;
  IP[l++] = reinterpret_cast<void *>(tluni_check_arg);
  IP[l++] = (void *)(arg->id);
  int y = gen_uni(arg->type, IP + l, max_size - l, vars);
  if (y < 0) {
    return -1;
  }
  l += y;
  return l;
}

int gen_uni(struct tl_tree *t, void **IP, int max_size, int *vars) {
  if (max_size <= 10) {
    return -1;
  }
  assert (t);
  int x = TL_TREE_METHODS (t)->type(t);
  int l = 0;
  int i;
  int j;
  tl_tree_type *t1;
  tl_tree_array *t2;
  int y;
  switch (x) {
    case NODE_TYPE_TYPE:
      t1 = (tl_tree_type *)t;
      IP[l++] = (void *)tluni_check_type;
      IP[l++] = ((struct tl_tree_type *)t)->type;
      for (i = 0; i < t1->children_num; i++) {
        y = gen_uni(t1->children[i], IP + l, max_size - l, vars);
        if (y < 0) {
          return -1;
        }
        l += y;
      }
      return l;
    case NODE_TYPE_NAT_CONST:
      IP[l++] = (void *)tluni_check_nat_const;
      IP[l++] = (void *)(t);
      return l;
    case NODE_TYPE_ARRAY:
      t2 = (tl_tree_array *)t;
      IP[l++] = (void *)tluni_check_array;
      IP[l++] = (void *)(t);
      y = gen_uni(t2->multiplicity, IP + l, max_size - l, vars);
      if (y < 0) {
        return -1;
      }
      l += y;
      for (i = 0; i < t2->args_num; i++) {
        y += gen_uni_arg(t2->args[i], IP + l, max_size - l, vars);
        if (y < 0) {
          return -1;
        }
        l += y;
      }
      return l;
    case NODE_TYPE_VAR_TYPE:
      i = ((struct tl_tree_var_type *)t)->var_num;
      if (!vars[i]) {
        IP[l++] = (void *) tluni_set_type_var;
        IP[l++] = (void *)(long)i;
//      IP[l ++] = (void *)(long)(t->flags & FLAGS_MASK);
        vars[i] = 1;
      } else if (vars[i] == 1) {
        IP[l++] = (void *) tluni_check_type_var;
        IP[l++] = (void *)(long)i;
//      IP[l ++] = (void *)(long)(t->flags & FLAGS_MASK);
      } else {
        return -1;
      }
      return l;
    case NODE_TYPE_VAR_NUM:
      i = ((struct tl_tree_var_num *)t)->var_num;
      j = ((struct tl_tree_var_num *)t)->dif;
      if (!vars[i]) {
        IP[l++] = (void *)tluni_set_nat_var;
        IP[l++] = (void *)(long)i;
        IP[l++] = (void *)(long)j;
        vars[i] = 2;
      } else if (vars[i] == 2) {
        IP[l++] = (void *)tluni_check_nat_var;
        IP[l++] = (void *)(long)i;
        IP[l++] = (void *)(long)j;
      } else {
        return -1;
      }
      return l;
    default:
      assert (0);
      return -1;
  }
}

int gen_create(struct tl_tree *t, void **IP, int max_size, int *vars) {
  if (max_size <= 10) {
    return -1;
  }
  int x = TL_TREE_METHODS (t)->type(t);
  int l = 0;
  if (!TL_IS_NAT_VAR (t) && (t->flags & FLAG_NOVAR)) {
    IP[l++] = (void *)tls_push;
//    TL_TREE_METHODS (t)->inc_ref (t);
    IP[l++] = (void *)(t);
    return l;
  }
  int i;
  int y;
  struct tl_tree_type *t1;
  struct tl_tree_array *t2;
  switch (x) {
    case NODE_TYPE_TYPE:
      t1 = (tl_tree_type *)t;
      for (i = 0; i < t1->children_num; i++) {
        y = gen_create(t1->children[i], IP + l, max_size - l, vars);
        if (y < 0) {
          return -1;
        }
        l += y;
      }
      if (l + 10 >= max_size) {
        return -1;
      }
      IP[l++] = (void *)tlsub_create_type;
      IP[l++] = (void *)(long)(t1->self.flags & FLAGS_MASK);
      IP[l++] = (void *)(t1->type);
      return l;
    case NODE_TYPE_NAT_CONST:
      IP[l++] = (void *)tls_push;
      IP[l++] = (void *)(t);
      return l;
    case NODE_TYPE_ARRAY:
      t2 = (tl_tree_array *)t;
      assert (t2->multiplicity);
      y = gen_create(t2->multiplicity, IP + l, max_size - l, vars);
      if (y < 0) {
        return -1;
      }
      l += y;

      for (i = 0; i < t2->args_num; i++) {
        assert (t2->args[i]);
        //y = gen_field_store (t2->args[i], IP + l, max_size - l, vars, i);
        y = gen_create(t2->args[i]->type, IP + l, max_size - l, vars);
        if (y < 0) {
          return -1;
        }
        l += y;
      }
      if (l + 10 + t2->args_num >= max_size) {
        return -1;
      }

      IP[l++] = (void *)tlsub_create_array;
      IP[l++] = (void *)(long)(t2->self.flags & FLAGS_MASK);
      IP[l++] = (void *)(long)t2->args_num;
      for (i = t2->args_num - 1; i >= 0; i--) {
        IP[l++] = (void *)(t2->args[i]->id);
      }
      return l;
    case NODE_TYPE_VAR_TYPE:
      IP[l++] = (void *)tlsub_push_type_var;
      IP[l++] = (void *)(long)((struct tl_tree_var_type *)t)->var_num;
      //IP[l ++] = (void *)(long)(t->flags & FLAGS_MASK);
      return l;
    case NODE_TYPE_VAR_NUM:
      IP[l++] = (void *)tlsub_push_nat_var;
      IP[l++] = (void *)(long)((struct tl_tree_var_num *)t)->var_num;
      IP[l++] = (void *)(long)((struct tl_tree_var_num *)t)->dif;
      return l;
    default:
      assert (0);
      return -1;
  }
}

int gen_field_store(struct arg *arg, void **IP, int max_size, int *vars, int num, int flat);

int gen_array_store(struct tl_tree_array *a, void **IP, int max_size, int *vars) {
  if (max_size <= 10) {
    return -1;
  }
  int l = 0;
  int i;
  if (a->args_num > 1) {
    for (i = 0; i < a->args_num; i++) {
      int x = gen_field_store(a->args[i], IP + l, max_size - l, vars, i, 0);
      if (x < 0) {
        return -1;
      }
      l += x;
    }
  } else {
    int x = gen_field_store(a->args[0], IP + l, max_size - l, vars, 0, 1);
    if (x < 0) {
      return -1;
    }
    l += x;
  }
  if (max_size - l <= 10) {
    return -1;
  }
  IP[l++] = (void *)tlsub_ret_ok;
//  c->IP = IP_dup (IP, l);
  return l;
}

int gen_field_fetch(struct arg *arg, void **IP, int max_size, int *vars, int num, int flat, const char *owner_name);

int gen_array_fetch(struct tl_tree_array *a, void **IP, int max_size, int *vars) {
  if (max_size <= 10) {
    return -1;
  }
  int l = 0;
  if (a->args_num > 1) {
    int i;
    for (i = 0; i < a->args_num; i++) {
      int x = gen_field_fetch(a->args[i], IP + l, max_size - l, vars, i, 0, "tl_array");
      if (x < 0) {
        return -1;
      }
      l += x;
    }
  } else {
    int x = gen_field_fetch(a->args[0], IP + l, max_size - l, vars, 0, 1, "tl_array");
    if (x < 0) {
      return -1;
    }
    l += x;
  }
  if (max_size - l <= 10) {
    return -1;
  }
  IP[l++] = (void *)tlsub_ret_ok;
//  c->IP = IP_dup (IP, l);
  return l;
}

int gen_constructor_store(struct tl_combinator *c, void **IP, int max_size);
int gen_constructor_fetch(struct tl_combinator *c, void **IP, int max_size);

static unsigned int get_magic_of_arg_type(struct arg *arg) {
  if (TYPE(arg->type) != NODE_TYPE_TYPE) {
    return 0;
  }
  struct tl_tree_type *type = (struct tl_tree_type *)arg->type;
  return type->type->name;
}

int gen_field_store(struct arg *arg, void **IP, int max_size, int *vars, int num, int flat) {
  assert (arg);
  if (max_size <= 10) {
    return -1;
  }
  int l = 0;
  if (arg->exist_var_num >= 0) {
    IP[l++] = (void *)tlcomb_check_bit;
    IP[l++] = (void *)(long)arg->exist_var_num;
    IP[l++] = (void *)(long)arg->exist_var_bit;
    IP[l++] = 0;
  }
  if (!flat) {
    IP[l++] = (void *)tlcomb_store_field;
    IP[l++] = (void *)(arg->id);
    IP[l++] = (void *)(long)num;
    IP[l++] = (void *)(long)get_magic_of_arg_type(arg);
  } else {
    assert (!num);
  }
  if (arg->var_num >= 0) {
    assert (TL_TREE_METHODS(arg->type)->type(arg->type) == NODE_TYPE_TYPE);
    int t = ((struct tl_tree_type *)arg->type)->type->name;
    if (t == NAME_VAR_TYPE) {
      IP[l++] = (void *) tlcomb_store_var_type;
      IP[l++] = (void *)(long)arg->var_num;
      IP[l++] = (void *)(long)(arg->type->flags & FLAGS_MASK);
    } else {
      assert (t == NAME_VAR_NUM);
      IP[l++] = (void *)tlcomb_store_var_num;
      IP[l++] = (void *)(long)arg->var_num;
    }
  } else {
    int t = TL_TREE_METHODS (arg->type)->type(arg->type);
    if ((t == NODE_TYPE_TYPE || t == NODE_TYPE_VAR_TYPE)) {
      tl_tree_type *t1 = (tl_tree_type *)(arg->type);

      if (0 && t == NODE_TYPE_TYPE && t1->type->arity == 0 && t1->type->constructors_num == 1) {
        if (!(t1->self.flags & FLAG_BARE)) {
          IP[l++] = (void *)tlcomb_store_const_int;
          IP[l++] = (void *)(long)t1->type->constructors[0]->name;
        }
        if (!t1->type->constructors[0]->IP_len) {
          if (gen_constructor_store(t1->type->constructors[0], IP + l, max_size - l) < 0) {
            return -1;
          }
        }
        void **_IP = t1->type->constructors[0]->IP;
        assert (_IP[0] == tluni_check_type);
        assert (_IP[1] == t1->type);
        if (l + t1->type->constructors[0]->IP_len + 10 > max_size) {
          return -1;
        }
        memcpy(IP + l, _IP + 2, sizeof(void *) * (t1->type->constructors[0]->IP_len - 2));

        l += t1->type->constructors[0]->IP_len - 2;
        assert (IP[l - 1] == tlsub_ret_ok);
        l--;
      } else {
        int r = gen_create(arg->type, IP + l, max_size - l, vars);
        if (r < 0) {
          return -1;
        }
        l += r;
        if (l + 10 > max_size) {
          return -1;
        }
        IP[l++] = (void *)tlcomb_store_type;
      }
    } else {
      assert (t == NODE_TYPE_ARRAY);
      int r = gen_create(((struct tl_tree_array *)arg->type)->multiplicity, IP + l, max_size - l, vars);
      if (r < 0) {
        return -1;
      }
      l += r;
      if (l + 10 > max_size) {
        return -1;
      }
      IP[l++] = (void *)tlcomb_store_array;
      void *newIP[1000];
      int v = gen_array_store(((struct tl_tree_array *)arg->type), newIP, 1000, vars);
      if (v < 0) {
        return -1;
      }
      IP[l++] = (void *)(IP_dup(newIP, v));
    }
  }
  if (l + 10 > max_size) {
    return -1;
  }
  if (!flat) {
    IP[l++] = (void *)tls_arr_pop;
  }
  if (arg->exist_var_num >= 0) {
    IP[3] = (void *)(long)(l - 4);
  }
  return l;
}

int gen_field_fetch(struct arg *arg, void **IP, int max_size, int *vars, int num, int flat, const char *owner_name) {
  assert (arg);
  if (max_size <= 30) {
    return -1;
  }
  int l = 0;
  if (arg->exist_var_num >= 0) {
    IP[l++] = (void *)tlcomb_check_bit;
    IP[l++] = (void *)(long)arg->exist_var_num;
    IP[l++] = (void *)(long)arg->exist_var_bit;
    IP[l++] = 0;
//    fprintf (stderr, "r = %d\n", l);
//    fprintf (stderr, "n = %ld\n", (long)(IP[1]));
//    fprintf (stderr, "b = %ld\n", (long)(IP[2]));
  }
  if (!flat) {
    IP[l++] = (void *)tls_arr_push;
  }
  if (arg->var_num >= 0) {
    assert (TL_TREE_METHODS(arg->type)->type(arg->type) == NODE_TYPE_TYPE);
    int t = ((struct tl_tree_type *)arg->type)->type->name;
    if (t == NAME_VAR_TYPE) {
      fprintf(stderr, "Not supported yet\n");
      assert (0);
    } else {
      assert (t == NAME_VAR_NUM);
      if (vars[arg->var_num] == 0) {
        IP[l++] = (void *)tlcomb_fetch_var_num;
        IP[l++] = (void *)(long)arg->var_num;
        vars[arg->var_num] = 2;
      } else if (vars[arg->var_num] == 2) {
        IP[l++] = (void *)tlcomb_fetch_check_var_num;
        IP[l++] = (void *)(long)arg->var_num;
      } else {
        return -1;
      }
    }
  } else {
    int t = TL_TREE_METHODS (arg->type)->type(arg->type);
    if (t == NODE_TYPE_TYPE || t == NODE_TYPE_VAR_TYPE) {
      auto *t1 = (tl_tree_type *)(arg->type);

      if (0 && t == NODE_TYPE_TYPE && t1->type->arity == 0 && t1->type->constructors_num == 1) {
        if (!(t1->self.flags & FLAG_BARE)) {
          IP[l++] = (void *)tlcomb_skip_const_int;
          IP[l++] = (void *)(long)t1->type->constructors[0]->name;
        }
        if (!t1->type->constructors[0]->fIP_len) {
          if (gen_constructor_fetch(t1->type->constructors[0], IP + l, max_size - l) < 0) {
            return -1;
          }
        }
        void **_IP = t1->type->constructors[0]->fIP;
        assert (_IP[0] == tluni_check_type);
        assert (_IP[1] == t1->type);
        if (l + t1->type->constructors[0]->fIP_len + 10 > max_size) {
          return -1;
        }
        memcpy(IP + l, _IP + 2, sizeof(void *) * (t1->type->constructors[0]->fIP_len - 2));

        l += t1->type->constructors[0]->fIP_len - 2;
        assert (IP[l - 1] == tlsub_ret_ok);
        l--;
      } else {
        int r = gen_create(arg->type, IP + l, max_size - l, vars);
        if (r < 0) {
          return -1;
        }
        l += r;
        if (l + 10 > max_size) {
          return -1;
        }
        IP[l++] = (void *)tlcomb_fetch_type;
      }
    } else {
      assert (t == NODE_TYPE_ARRAY);
      struct tl_tree_array *array_tree = (struct tl_tree_array *)arg->type;
      int r = gen_create(array_tree->multiplicity, IP + l, max_size - l, vars);
      if (r < 0) {
        return -1;
      }
      l += r;
      if (l + 10 > max_size) {
        return -1;
      }
      IP[l++] = (void *)tlcomb_fetch_array;
      IP[l++] = (void *)(long)num;
      IP[l++] = (void *)(long)(array_tree->args_num == 1);
      IP[l++] = (void *)(owner_name);
      void *newIP[1000];
      int v = gen_array_fetch(array_tree, newIP, 1000, vars);
      if (v < 0) {
        return -1;
      }
      IP[l++] = (void *)(IP_dup(newIP, v));
    }
  }
  if (l + 10 > max_size) {
    return -1;
  }
  if (!flat) {
    IP[l++] = (void *)tlcomb_fetch_field_end;
    IP[l++] = (void *)(arg->id);
    IP[l++] = (void *)(long)num;
  }
  if (arg->exist_var_num >= 0) {
    IP[3] = (void *)(long)(l - 4);
//    fprintf (stderr, "r = %d\n", l);
//    fprintf (stderr, "n = %ld\n", (long)(IP[1]));
//    fprintf (stderr, "b = %ld\n", (long)(IP[2]));
  }
  return l;
}

int gen_field_excl_store(struct arg *arg, void **IP, int max_size, int *vars, int num) {
  assert (arg);
  if (max_size <= 10) {
    return -1;
  }
  int l = 0;
  IP[l++] = (void *)tlcomb_store_field;
  IP[l++] = (void *)(arg->id);
  IP[l++] = (void *)(long)num;
  IP[l++] = (void *)(long)get_magic_of_arg_type(arg);

  //fprintf (stderr, "arg->var_num = %d, arg->flags = %x\n", arg->var_num, arg->flags);
  assert (arg->var_num < 0);
  int t = TL_TREE_METHODS (arg->type)->type(arg->type);
  assert (t == NODE_TYPE_TYPE || t == NODE_TYPE_VAR_TYPE);
  IP[l++] = (void *)tlcomb_store_any_function;
  int x = gen_uni(arg->type, IP + l, max_size - l, vars);
  if (x < 0) {
    return -1;
  }
  l += x;
  if (max_size - l <= 10) {
    return -1;
  }

  IP[l++] = (void *)tls_arr_pop;
  return l;
}


int gen_constructor_store(struct tl_combinator *c, void **IP, int max_size) {
  assert (IP);
  if (c->IP) {
    return c->IP_len;
  }
  if (max_size <= 10) {
    return -1;
  }
  int l = 0;
  assert (!c->IP);
  int vars[c->var_num];
  memset(vars, 0, sizeof(int) * c->var_num);
  int x = gen_uni(c->result, IP + l, max_size - l, vars);
  if (x < 0) {
    return -1;
  }
  l += x;
  switch(c->name) {
    case NAME_INT: {
      IP[l++] = (void *)(tlcomb_store_int);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_LONG: {
      IP[l++] = (void *)(tlcomb_store_long);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_STRING: {
      IP[l++] = (void *)(tlcomb_store_string);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_DOUBLE: {
      IP[l++] = (void *)(tlcomb_store_double);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_FLOAT: {
      IP[l++] = (void *)(tlcomb_store_float);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_MAYBE_FALSE: {
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_MAYBE_TRUE: {
      IP[l++] = (void *)(tlcomb_store_maybe);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_store_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *) IP_dup(tIP, 4);
      IP[l++] = (void *) tlsub_ret_ok;
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_VECTOR: {
      IP[l++] = (void *)(tlcomb_store_vector);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_store_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_DICTIONARY: {
      IP[l++] = (void *)(tlcomb_store_dictionary);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_store_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_INT_KEY_DICTIONARY: {
      IP[l++] = (void *)(tlcomb_store_int_key_dictionary);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_store_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
    case NAME_LONG_KEY_DICTIONARY: {
      IP[l++] = (void *)(tlcomb_store_long_key_dictionary);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_store_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->IP = IP_dup(IP, l);
      c->IP_len = l;
      return l;
    }
  }
  if (verbosity >= 2) {
    fprintf(stderr, "c->id = %s, c->args_num = %d, c->args = %p\n", c->id, c->args_num, c->args);
  }

  int arg_idx = -1;
  assert(TYPE (c->result) == NODE_TYPE_TYPE);
  const struct tl_type *t = ((struct tl_tree_type *)c->result)->type;
  if (is_tl_type_flat(t, &arg_idx)) {
    x = gen_field_store(c->args[arg_idx], IP + l, max_size - l, vars, 0, 1);
    if (x < 0) {
      return -1;
    }
    l += x;
    if (max_size - l <= 10) {
      return -1;
    }
    IP[l++] = (void *)(tlsub_ret_ok);
    c->IP = IP_dup(IP, l);
    c->IP_len = l;
    return l;
  }

  assert (!c->args_num || (c->args && c->args[0]));
  for (int i = 0; i < c->args_num; i++) {
    if (!(c->args[i]->flags & FLAG_OPT_VAR) && !is_arg_named_fields_mask_bit(c->args[i])) {
//  fprintf (stderr, "%d\n", __LINE__);
      x = gen_field_store(c->args[i], IP + l, max_size - l, vars, i + 1, 0);
//  fprintf (stderr, "%d\n", __LINE__);
      if (x < 0) {
        return -1;
      }
      l += x;
//    fprintf (stderr, ".");
    }
  }
  if (max_size - l <= 10) {
    return -1;
  }
  IP[l++] = (void *)(tlsub_ret_ok);
  c->IP = IP_dup(IP, l);
  c->IP_len = l;
  return l;
}

int gen_function_store(struct tl_combinator *c, void **IP, int max_size) {
  if (max_size <= 10) {
    return -1;
  }
  assert (!c->IP);
  int l = 0;
  IP[l++] = (void *)(tls_store_int);
  IP[l++] = (void *)(long)c->name;
  int i;
  int vars[c->var_num];
  memset(vars, 0, sizeof(int) * c->var_num);
  int x;
  for (i = 0; i < c->args_num; i++) {
    if (!(c->args[i]->flags & FLAG_OPT_VAR) && !is_arg_named_fields_mask_bit(c->args[i])) {
      if (c->args[i]->flags & FLAG_EXCL) {
        x = gen_field_excl_store(c->args[i], IP + l, max_size - l, vars, i + 1);
      } else {
//      fprintf (stderr, "(");
        x = gen_field_store(c->args[i], IP + l, max_size - l, vars, i + 1, 0);
      }
      if (x < 0) {
        return -1;
      }
      l += x;
//    fprintf (stderr, ".");
    }
  }
  int r = gen_create(c->result, IP + l, max_size - l, vars);
  if (r < 0) {
    return -1;
  }
  l += r;
  if (max_size - l <= 10) {
    return -1;
  }
  IP[l++] = (void *)(tlsub_ret);
  c->IP = IP_dup(IP, l);
  c->IP_len = l;
  return l;
}

int gen_constructor_fetch(struct tl_combinator *c, void **IP, int max_size) {
  if (c->fIP) {
    return c->fIP_len;
  }
  if (max_size <= 10) {
    return -1;
  }
  int l = 0;
  assert (!c->fIP);
  int i;
  int vars[c->var_num];
  memset(vars, 0, sizeof(int) * c->var_num);
  int x = gen_uni(c->result, IP + l, max_size - l, vars);
  if (x < 0) {
    return -1;
  }
  l += x;
  switch (c->name) {
    case NAME_INT: {
      IP[l++] = (void *)(tlcomb_fetch_int);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_LONG: {
      IP[l++] = (void *)(tlcomb_fetch_long);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_STRING: {
      IP[l++] = (void *)(tlcomb_fetch_string);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_DOUBLE: {
      IP[l++] = (void *)(tlcomb_fetch_double);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_FLOAT: {
      IP[l++] = (void *)(tlcomb_fetch_float);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_VECTOR: {
      IP[l++] = (void *)(tlcomb_fetch_vector);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_fetch_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_DICTIONARY: {
      IP[l++] = (void *)(tlcomb_fetch_dictionary);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_fetch_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_INT_KEY_DICTIONARY: {
      IP[l++] = (void *)(tlcomb_fetch_int_key_dictionary);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_fetch_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_LONG_KEY_DICTIONARY: {
      IP[l++] = (void *)(tlcomb_fetch_long_key_dictionary);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_fetch_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_MAYBE_TRUE: {
      IP[l++] = (void *)(tlcomb_fetch_maybe);
      static void *tIP[4];
      tIP[0] = (void *)(tlsub_push_type_var);
      tIP[1] = (void *)((long)0);
      tIP[2] = (void *)(tlcomb_fetch_type);
      tIP[3] = (void *)(tlsub_ret_ok);
      IP[l++] = (void *)(IP_dup(tIP, 4));
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_MAYBE_FALSE: {
      IP[l++] = (void *)(tlcomb_fetch_maybe_false);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_BOOL_FALSE: {
      IP[l++] = (void *)(tlcomb_fetch_false);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
    case NAME_BOOL_TRUE: {
      IP[l++] = (void *)(tlcomb_fetch_true);
      IP[l++] = (void *)(tlsub_ret_ok);
      c->fIP = IP_dup(IP, l);
      c->fIP_len = l;
      return l;
    }
  }
  int arg_idx = -1;
  assert(TYPE (c->result) == NODE_TYPE_TYPE);
  const struct tl_type *t = ((struct tl_tree_type *)c->result)->type;
  if (is_tl_type_flat(t, &arg_idx)) {
    x = gen_field_fetch(c->args[arg_idx], IP + l, max_size - l, vars, arg_idx + 1, 1, c->id);
    if (x < 0) {
      return -1;
    }
    l += x;
    if (max_size - l <= 10) {
      return -1;
    }
    IP[l++] = (void *)(tlsub_ret_ok);
    c->fIP = IP_dup(IP, l);
    c->fIP_len = l;
    return l;
  }
  for (i = 0; i < c->args_num; i++) {
    if (!(c->args[i]->flags & FLAG_OPT_VAR)) {
      x = gen_field_fetch(c->args[i], IP + l, max_size - l, vars, i + 1, 0, c->id);
      if (x < 0) {
        return -1;
      }
      l += x;
//    fprintf (stderr, ".");
    }
  }
  if (max_size - l <= 10) {
    return -1;
  }
  IP[l++] = (void *)(tlsub_ret_ok);
  c->fIP = IP_dup(IP, l);
  c->fIP_len = l;
  return l;
}

int gen_function_fetch(void **IP, int max_size) {
  if (max_size <= 10) {
    return -1;
  }
  int l = 0;
  IP[l++] = (void *)(tlcomb_fetch_type);
  IP[l++] = (void *)(tlsub_ret_ok);
  fIP = IP_dup(IP, l);
  return 2;
}
/* }}} */

/* {{{ Destructors */
int tl_constructors;
int tl_types;
int tl_functions;

void delete_combinator(struct tl_combinator *c) {
  if (!c) {
    return;
  }
  if (c->is_fun) {
    tl_functions--;
  } else {
    tl_constructors--;
  }
  if (c->IP) {
    zzfree(c->IP, c->IP_len);
    ADD_PFREE (c->IP_len);
  }
  if (c->fIP) {
    zzfree(c->fIP, c->fIP_len);
    ADD_PFREE (c->fIP_len);
  }
  if (c->result) {
    DEC_REF (c->result);
  }
  if (c->id) {
    ADD_PFREE (strlen(c->id));
    zzstrfree(c->id);
  }
  int i;
  if (c->args_num && c->args) {
    for (i = 0; i < c->args_num; i++) {
      if (c->args[i]) {
        if (c->args[i]->type) {
          DEC_REF (c->args[i]->type);
        }
        zzfree(c->args[i], sizeof(*c->args[i]));
        ADD_PFREE (sizeof(*c->args[i]));
      }
    }
    zzfree(c->args, sizeof(void *) * c->args_num);
    ADD_PFREE (sizeof(void *) * c->args_num);
  }
  zzfree(c, sizeof(*c));
  ADD_PFREE (sizeof(*c));
}

void delete_type(struct tl_type *t) {
  if (!t) {
    return;
  }
  if (t->id) {
    ADD_PFREE (strlen(t->id));
    zzstrfree(t->id);
  }
  if (t->constructors_num && t->constructors) {
    int i;
    for (i = 0; i < t->extra; i++) {
      delete_combinator(t->constructors[i]);
    }
    zzfree(t->constructors, t->constructors_num * sizeof(void *));
    ADD_PFREE (t->constructors_num * sizeof(void *));
  }
  tl_types--;
  zzfree(t, sizeof(*t));
  ADD_PFREE (sizeof(*t));
}

void tl_config_delete(struct tl_config *config) {
  config_list[config->pos] = 0;
  int size = sizeof(struct hash_table_tl_fun_id);
  hash_clear_tl_fun_id(config->ht_fid);
  hash_clear_tl_type_id(config->ht_tid);
  hash_clear_tl_fun_name(config->ht_fname);
  hash_clear_tl_type_name(config->ht_tname);
  zzfree(config->ht_tid->E, sizeof(void *) * (1 << 12));
  zzfree(config->ht_tid, size);
  zzfree(config->ht_fid->E, sizeof(void *) * (1 << 12));
  zzfree(config->ht_fid, size);
  zzfree(config->ht_tname->E, sizeof(void *) * (1 << 12));
  zzfree(config->ht_tname, size);
  zzfree(config->ht_fname->E, sizeof(void *) * (1 << 12));
  zzfree(config->ht_fname, size);
  ADD_PFREE ((size + (1 << 12) * sizeof(void *)) * 4);
  int i;
  for (i = 0; i < config->tn; i++) {
    if (config->tps[i]) {
      delete_type(config->tps[i]);
    }
  }
  zzfree(config->tps, config->tn * sizeof(void *));
  ADD_PFREE (config->tn * sizeof(void *));
  for (i = 0; i < config->fn; i++) {
    if (config->fns[i]) {
      delete_combinator(config->fns[i]);
    }
  }
  zzfree(config->fns, config->fn * sizeof(void *));
  ADD_PFREE (config->fn * sizeof(void *));

  zzfree(config, sizeof(*config));
  ADD_PFREE (sizeof(*config));
}

void tl_delete_old_configs() {
  int i;
  for (i = 0; i < CONFIG_LIST_SIZE; i++) {
    if (config_list[i] && config_list[i] != cur_config) {
      tl_config_delete(config_list[i]);
    }
  }
}

void tl_config_alloc() {
  cur_config = reinterpret_cast<tl_config *>(zzmalloc0(sizeof(*cur_config)));
  ADD_PMALLOC (sizeof(*cur_config));
  config_list_pos++;
  if (config_list_pos >= CONFIG_LIST_SIZE) {
    config_list_pos -= CONFIG_LIST_SIZE;
  }
  config_list[config_list_pos] = cur_config;
  int size = sizeof(struct hash_table_tl_fun_id);
  cur_config->ht_tid = reinterpret_cast<hash_table_tl_type_id *>(zzmalloc(size));
  cur_config->ht_tid->size = (1 << 12);
  cur_config->ht_tid->mask = (1 << 12) - 1;
  cur_config->ht_tid->E = reinterpret_cast<hash_elem_tl_type_id **>(zzmalloc0(sizeof(hash_elem_tl_type_id *) * (1 << 12)));
  cur_config->ht_fid = reinterpret_cast<hash_table_tl_fun_id*>(zzmalloc(size));
  cur_config->ht_fid->size = (1 << 12);
  cur_config->ht_fid->mask = (1 << 12) - 1;
  cur_config->ht_fid->E = reinterpret_cast<hash_elem_tl_fun_id **>(zzmalloc0(sizeof(hash_elem_tl_fun_id *) * (1 << 12)));
  cur_config->ht_tname = reinterpret_cast<hash_table_tl_type_name *>(zzmalloc(size));
  cur_config->ht_tname->size = (1 << 12);
  cur_config->ht_tname->mask = (1 << 12) - 1;
  cur_config->ht_tname->E = reinterpret_cast<hash_elem_tl_type_name **>(zzmalloc0(sizeof(void *) * (1 << 12)));
  cur_config->ht_fname = reinterpret_cast<hash_table_tl_fun_name *>(zzmalloc(size));
  cur_config->ht_fname->size = (1 << 12);
  cur_config->ht_fname->mask = (1 << 12) - 1;
  cur_config->ht_fname->E = reinterpret_cast<hash_elem_tl_fun_name **>(zzmalloc0(sizeof(hash_elem_tl_fun_name *) * (1 << 12)));
  cur_config->pos = config_list_pos;

  ADD_PMALLOC (size * 4);
  ADD_PMALLOC (sizeof(void *) * (1 << 12) * 4);
}

void tl_config_back() {
  tl_config_delete(cur_config);
  config_list_pos--;
  if (config_list_pos < 0) {
    config_list_pos += CONFIG_LIST_SIZE;
  }
  cur_config = config_list[config_list_pos];
}
/* }}} */

/* read_cfg {{{ */
int __fd;
unsigned long long config_crc64 = -1;
int schema_version;
/*
int read_int () {
  int x = 0;
  assert (read (__fd, &x, 4) == 4);
  config_crc64 = ~crc64_partial (&x, 4, ~config_crc64);
  return x;
}

int lookup_int () {
  int x = 0;
  assert (read (__fd, &x, 4) == 4);
  lseek (__fd, -4, SEEK_CUR);
  return x;
}

int try_read_int () {
  int x = 0;
  int z = read (__fd, &x, 4);
  if (z == 4) {
    config_crc64 = ~crc64_partial (&x, 4, ~config_crc64);
    return x;
  } else {
    return 0;
  }
}

long long read_long () {
  long long x = 0;
  assert (read (__fd, &x, 8) == 8);
  config_crc64 = ~crc64_partial (&x, 8, ~config_crc64);
  return x;
}

char *read_string () {
  if (schema_version < 1) {
    int x = read_int ();
    if (x == 0) { return 0; }
    char *s = zzmalloc (x + 1);
    ADD_PMALLOC (x + 1);
    s[x] = 0;
    assert (read (__fd, s, x) == x);
    config_crc64 = ~crc64_partial (s, x, ~config_crc64);
    return s;
  } else {
    int x = read_char ();
  }
}*/

struct tl_tree *read_tree(int *var_num);
struct tl_tree *read_nat_expr(int *var_num);
struct tl_tree *read_type_expr(int *var_num);
int read_args_list(struct arg **args, int args_num, int *var_num);

tl_tree *read_num_const(int *var_num) {
  auto *res = (tl_tree *)int_to_var_nat_const_init(schema_version >= 2 ? tl_parse_int() : tl_parse_long());
  return tl_parse_error() ? nullptr : res;
}

tl_tree *read_num_var(int *var_num) {
  auto *T = reinterpret_cast<tl_tree_var_num *>(zzmalloc(sizeof(tl_tree_var_num)));
  ADD_PMALLOC (sizeof(*T));
  T->self.flags = 0;
  T->self.ref_cnt = 1;
  total_ref_cnt++;
  persistent_tree_nodes++;
  total_tree_nodes_existed++;
  T->self.methods = &tl_pvar_num_methods;
  if (schema_version >= 2) {
    T->dif = tl_parse_int();
  } else {
    T->dif = tl_parse_long();
  }
  T->var_num = tl_parse_int();
  if (tl_parse_error()) {
    return 0;
  }
  if (T->var_num >= *var_num) {
    *var_num = T->var_num + 1;
  }
  if (T->self.flags & FLAG_NOVAR) {
    return 0;
  }
  return (tl_tree *)T;
}

struct tl_tree *read_type_var(int *var_num) {
  auto *T = reinterpret_cast<tl_tree_var_type *>(zzmalloc0(sizeof(tl_tree_var_type)));
  ADD_PMALLOC (sizeof(*T));
//  T->self.flags = 0;
  T->self.ref_cnt = 1;
  total_ref_cnt++;
  persistent_tree_nodes++;
  total_tree_nodes_existed++;
  T->self.methods = &tl_pvar_type_methods;
  T->var_num = tl_parse_int();
  T->self.flags = tl_parse_int();
  if (tl_parse_error()) {
    return 0;
  }
  if (T->var_num >= *var_num) {
    *var_num = T->var_num + 1;
  }
  if (T->self.flags & (FLAG_NOVAR | FLAG_BARE)) {
    return 0;
  }
  return reinterpret_cast<tl_tree *>(T);
}

tl_tree *read_array(int *var_num) {
  auto *T = reinterpret_cast<tl_tree_array *>(zzmalloc0(sizeof(tl_tree_array)));
  ADD_PMALLOC (sizeof(*T));
  T->self.ref_cnt = 1;
  total_ref_cnt++;
  persistent_tree_nodes++;
  total_tree_nodes_existed++;
  T->self.methods = &tl_parray_methods;
  T->self.flags = 0;
  if (schema_version >= 2) {
    T->multiplicity = read_nat_expr(var_num);
  } else {
    T->multiplicity = read_tree(var_num);
  }
  if (!T->multiplicity) {
    return 0;
  }
  T->args_num = tl_parse_int();
  if (T->args_num <= 0 || T->args_num > 1000 || tl_parse_error()) {
    return 0;
  }
  T->args = reinterpret_cast<arg **>(zzmalloc0(sizeof(void *) * T->args_num));
  ADD_PMALLOC (sizeof(void *) * T->args_num);
  if (read_args_list(T->args, T->args_num, var_num) < 0) {
    return 0;
  }
  T->self.flags |= FLAG_NOVAR;
  int i;
  for (i = 0; i < T->args_num; i++) {
    if (!(T->args[i]->flags & FLAG_NOVAR)) {
      T->self.flags &= ~FLAG_NOVAR;
    }
  }
  return reinterpret_cast<tl_tree *>(T);
}

struct tl_tree *read_type(int *var_num) {
  auto *T = reinterpret_cast<tl_tree_type *>(zzmalloc0(sizeof(tl_tree_type)));
  ADD_PMALLOC (sizeof(*T));
  T->self.ref_cnt = 1;
  total_ref_cnt++;
  persistent_tree_nodes++;
  total_tree_nodes_existed++;
  T->self.methods = &tl_ptype_methods;

  T->type = tl_type_get_by_name(tl_parse_int());
  if (!T->type) {
    return 0;
  }
  T->self.flags = tl_parse_int();
  T->children_num = tl_parse_int();
  if (verbosity >= 2) {
    fprintf(stderr, "T->flags = %d, T->children_num = %d\n", T->self.flags, T->children_num);
  }
  if (tl_parse_error() || T->type->arity != T->children_num) {
    return 0;
  }
  if (T->children_num < 0 || T->children_num > 1000) {
    return 0;
  }
  T->children = reinterpret_cast<tl_tree **>(zzmalloc0(sizeof(tl_tree *) * T->children_num));
  ADD_PMALLOC (sizeof(void *) * T->children_num);
  int i;
  T->self.flags |= FLAG_NOVAR;
  for (i = 0; i < T->children_num; i++) {
    if (schema_version >= 2) {
      auto t = static_cast<unsigned int>(tl_parse_int());
      if (tl_parse_error()) {
        return 0;
      }
      if (t == TLS_EXPR_NAT) {
        if (!(T->type->params_types & (1 << i))) {
          return 0;
        }
        T->children[i] = read_nat_expr(var_num);
      } else if (t == TLS_EXPR_TYPE) {
        if ((T->type->params_types & (1 << i))) {
          return 0;
        }
        T->children[i] = read_type_expr(var_num);
      } else {
        return 0;
      }
    } else {
      T->children[i] = read_tree(var_num);
    }
    if (!T->children[i]) {
      return 0;
    }
    if (!TL_IS_NAT_VAR (T->children[i]) && !(T->children[i]->flags & FLAG_NOVAR)) {
      T->self.flags &= ~FLAG_NOVAR;
    }
  }
  return reinterpret_cast<tl_tree *>(T);
}

struct tl_tree *read_tree(int *var_num) {
  auto x = static_cast<unsigned int>(tl_parse_int());
  if (verbosity >= 2) {
    fprintf(stderr, "read_tree: constructor = 0x%08x\n", x);
  }
  switch (x) {
    case TLS_TREE_NAT_CONST:
      return read_num_const(var_num);
    case TLS_TREE_NAT_VAR:
      return read_num_var(var_num);
    case TLS_TREE_TYPE_VAR:
      return read_type_var(var_num);
    case TLS_TREE_TYPE:
      return read_type(var_num);
    case TLS_TREE_ARRAY:
      return read_array(var_num);
    default:
      if (verbosity) {
        fprintf(stderr, "x = 0x%08x\n", x);
      }
      return 0;
  }
}

struct tl_tree *read_type_expr(int *var_num) {
  auto x = static_cast<unsigned int>(tl_parse_int());
  if (verbosity >= 2) {
    fprintf(stderr, "read_type_expr: constructor = 0x%08x\n", x);
  }
  switch (x) {
    case TLS_TYPE_VAR:
      return read_type_var(var_num);
    case TLS_TYPE_EXPR:
      return read_type(var_num);
    case TLS_ARRAY:
      return read_array(var_num);
    default:
      if (verbosity) {
        fprintf(stderr, "x = %d\n", x);
      }
      return 0;
  }
}

struct tl_tree *read_nat_expr(int *var_num) {
  auto x = static_cast<unsigned int>(tl_parse_int());
  if (verbosity >= 2) {
    fprintf(stderr, "read_nat_expr: constructor = 0x%08x\n", x);
  }
  switch (x) {
    case TLS_NAT_CONST:
      return read_num_const(var_num);
    case TLS_NAT_VAR:
      return read_num_var(var_num);
    default:
      if (verbosity) {
        fprintf(stderr, "x = 0x%08x\n", x);
      }
      return 0;
  }
}

struct tl_tree *read_expr(int *var_num) {
  auto x = static_cast<unsigned int>(tl_parse_int());
  if (verbosity >= 2) {
    fprintf(stderr, "read_nat_expr: constructor = 0x%08x\n", x);
  }
  switch (x) {
    case TLS_EXPR_NAT:
      return read_nat_expr(var_num);
    case TLS_EXPR_TYPE:
      return read_type_expr(var_num);
    default:
      if (verbosity) {
        fprintf(stderr, "x = %d\n", x);
      }
      return 0;
  }
}

int read_args_list(arg **args, int args_num, int *var_num) {
  int i;
  for (i = 0; i < args_num; i++) {
    args[i] = reinterpret_cast<arg *>(zzmalloc0(sizeof(arg)));
    args[i]->exist_var_num = -1;
    args[i]->exist_var_bit = 0;
    ADD_PMALLOC (sizeof(struct arg));
    if (schema_version == 1) {
      if (tl_parse_int() != TLS_ARG) {
        return -1;
      }
    } else {
      if (tl_parse_int() != TLS_ARG_V2) {
        return -1;
      }
    }
    if (tl_parse_string(&args[i]->id) < 0) {
      return -1;
    }
    ADD_MALLOC (strlen(args[i]->id));
    ADD_PMALLOC (strlen(args[i]->id));
    args[i]->flags = tl_parse_int();
    if (schema_version >= 3) {
      int x = args[i]->flags & 6;
      args[i]->flags &= ~6;
      if (x & 2) {
        args[i]->flags |= 4;
      }
      if (x & 4) {
        args[i]->flags |= 2;
      }
    }
    if (schema_version >= 2) {
      if (args[i]->flags & 2) {
        args[i]->flags &= ~2;
        args[i]->flags |= (1 << 20);
      }
      if (args[i]->flags & 4) {
        args[i]->flags &= ~4;
        args[i]->var_num = tl_parse_int();
      } else {
        args[i]->var_num = -1;
      }
    } else {
      args[i]->var_num = tl_parse_int();
    }
    if (args[i]->var_num >= *var_num) {
      *var_num = args[i]->var_num + 1;
    }
    if (args[i]->flags & FLAG_OPT_FIELD) {
      args[i]->exist_var_num = tl_parse_int();
      args[i]->exist_var_bit = tl_parse_int();
    }
    if (schema_version >= 2) {
      args[i]->type = read_type_expr(var_num);
    } else {
      args[i]->type = read_tree(var_num);
    }
    if (!args[i]->type) {
      return -1;
    }
    if (args[i]->var_num < 0 && args[i]->exist_var_num < 0 && (TL_IS_NAT_VAR(args[i]->type) || (args[i]->type->flags & FLAG_NOVAR))) {
      args[i]->flags |= FLAG_NOVAR;
    }
    static char sflags[2000];
    static int flags_v[7] = {
      FLAG_OPT_VAR,
      FLAG_EXCL,
      FLAG_OPT_FIELD,
      FLAG_NOVAR,
      FLAG_BARE,
      FLAG_DEFAULT_CONSTRUCTOR,
      FLAG_NOCONS
    };
    static const char *flags_n[7] = {
      "FLAG_OPT_VAR",
      "FLAG_EXCL",
      "FLAG_OPT_FIELD",
      "FLAG_NOVAR",
      "FLAG_BARE",
      "FLAG_DEFAULT_CONSTRUCTOR",
      "FLAG_NOCONS"
    };
    int pp = 0, j;
    sflags[0] = 0;
    for (j = 0; j < 7; ++j) {
      if ((args[i]->flags & flags_v[j]) != 0) {
        pp += snprintf(sflags + pp, 2000 - pp, "%s, ", flags_n[j]);
      }
    }
  }
  return 1;
}

int read_combinator_args_list(struct tl_combinator *c) {
  c->args_num = tl_parse_int();
  if (verbosity >= 2) {
    fprintf(stderr, "c->id = %s, c->args_num = %d\n", c->id, c->args_num);
  }
  if (c->args_num < 0 || c->args_num > 1000) {
    return -1;
  }
  c->args = reinterpret_cast<arg **>(zzmalloc0(sizeof(arg *) * c->args_num));
  c->var_num = 0;
  ADD_PMALLOC (sizeof(void *) * c->args_num);
  return read_args_list(c->args, c->args_num, &c->var_num);
}

int read_combinator_right(struct tl_combinator *c) {
  if (schema_version >= 2) {
    if (tl_parse_int() != TLS_COMBINATOR_RIGHT_V2 || tl_parse_error()) {
      return -1;
    }
    c->result = read_type_expr(&c->var_num);
  } else {
    if (static_cast<unsigned int>(tl_parse_int()) != TLS_COMBINATOR_RIGHT || tl_parse_error()) {
      return -1;
    }
    c->result = read_tree(&c->var_num);
  }
  if (!c->result) {
    return -1;
  }
  return 1;
}

int read_combinator_left(struct tl_combinator *c) {

  auto x = static_cast<unsigned int>(tl_parse_int());
  if (tl_parse_error()) {
    return -1;
  }

  if (x == TLS_COMBINATOR_LEFT_BUILTIN) {
    c->args_num = 0;
    c->var_num = 0;
    c->args = 0;
    return 1;
  } else if (x == TLS_COMBINATOR_LEFT) {
    return read_combinator_args_list(c);
  } else {
    return -1;
  }
}

static bool check_constructor(struct tl_combinator *constructor) {
  // проверяем, чтобы не было такого
  // test {t1:Type} {t2:Type} ... = Test t2 t1
  // и вообще любого расхождения в порядке
  // arg_idx = child_idx для всех переменных типа Type
  assert (TYPE(constructor->result) == NODE_TYPE_TYPE);
  struct tl_tree_type *res_tree = (struct tl_tree_type *)constructor->result;
  for (int i = 0; i < res_tree->children_num; ++i) {
    if (TYPE(res_tree->children[i]) == NODE_TYPE_VAR_TYPE) {
      struct tl_tree_var_type *as_type_var = (struct tl_tree_var_type *)res_tree->children[i];
      if (as_type_var->var_num != i) {
        fprintf(stderr, "Incorrect constructor! type vars order in tree and in args must be equal!\n");
        return false;
      }
    }
  }
  return true;
}

static bool check_type(struct tl_type *type) {
  if (is_php_array(type)) {
    return true;
  }
  if (is_tl_type_flat(type, NULL)) {
    struct tl_combinator *ctor = type->constructors[0];
    for (int i = 0; i < ctor->args_num; ++i) {
      struct tl_tree *arg_type = ctor->args[i]->type;
      if (TYPE(arg_type) == NODE_TYPE_TYPE) {
        if (!strcmp(((struct tl_tree_type *)arg_type)->type->id, "Type")) {
          fprintf(stderr, "Template flat TL types are prohibited!\n");
          return false;
        }
      }
    }
  }
  return true;
}

struct tl_combinator *read_combinators(int v) {
  auto *c = reinterpret_cast<tl_combinator *>(zzmalloc0(sizeof(tl_combinator)));
  c->name = tl_parse_int();
  if (tl_parse_error() || tl_parse_string(&c->id) < 0) {
    zzfree(c, sizeof(*c));
    return 0;
  }
  ADD_MALLOC (strlen(c->id));
  ADD_PMALLOC (strlen(c->id));
  int x = tl_parse_int();
  struct tl_type *t = tl_type_get_by_name(x);
  if (!t && (x || v != 3)) {
    ADD_PFREE (strlen(c->id));
    zzfree(c->id, strlen(c->id));
    zzfree(c, sizeof(*c));
    return 0;
  }
  assert (t || (!x && v == 3));
  if (v == 2) {
    if (t->extra >= t->constructors_num) {
      zzfree(c, sizeof(*c));
      return 0;
    }
    assert (t->extra < t->constructors_num);
    t->constructors[t->extra++] = c;
    tl_constructors++;
    c->is_fun = 0;
  } else {
    assert (v == 3);
    tl_fun_insert_id(c);
    tl_fun_insert_name(c);
    tl_functions++;
    c->is_fun = 1;
  }
  if (read_combinator_left(c) < 0) {
    //delete_combinator (c);
    return 0;
  }
  if (read_combinator_right(c) < 0) {
    //delete_combinator (c);
    return 0;
  }
  ADD_PMALLOC (sizeof(*c));
  if (schema_version >= 4) {
    c->flags = tl_parse_int();
  }
  if (!c->is_fun) {
    if (!check_constructor(c)) {
      return 0;
    }
  }
  return c;
}

struct tl_type *read_types() {
  auto *t = reinterpret_cast<tl_type *>(zzmalloc0(sizeof(tl_type)));
  t->name = tl_parse_int();
  if (tl_parse_error() || tl_parse_string(&t->id) < 0) {
    zzfree(t, sizeof(*t));
    return 0;
  }
  ADD_MALLOC (strlen(t->id));
  ADD_PMALLOC (strlen(t->id));

  t->constructors_num = tl_parse_int();
  if (tl_parse_error() || t->constructors_num < 0 || t->constructors_num > 1000) {
    free(t->id);
    zzfree(t, sizeof(*t));
    return 0;
  }
  t->constructors = reinterpret_cast<tl_combinator **>(zzmalloc0(sizeof(tl_combinator *) * t->constructors_num));
  t->flags = tl_parse_int();
  if (!strcmp(t->id, "Maybe") || !strcmp(t->id, "Bool")) {
    t->flags |= FLAG_NOCONS;
  }
  t->arity = tl_parse_int();
  t->params_types = tl_parse_long(); // params_types
  t->extra = 0;
  if (tl_parse_error()) {
    free(t->id);
    zzfree(t->constructors, sizeof(void *) * t->constructors_num);
    zzfree(t, sizeof(*t));
    return 0;
  }
  tl_type_insert_name(t);
  tl_type_insert_id(t);
  tl_types++;
  ADD_PMALLOC (sizeof(*t));
  ADD_PMALLOC (sizeof(void *) * t->constructors_num);
  //fprintf (stderr, "Adding type %s. Name %d\n", t->id, t->name);
  return t;
}


int MAGIC = 0x850230aa;
char *tl_config_name = 0;
int tl_config_date = 0;
int tl_config_version = 0;
time_t tl_config_mtime = 0;

int get_schema_version(unsigned int a) {
  if (a == TLS_SCHEMA) {
    return 1;
  }
  if (a == TLS_SCHEMA_V2) {
    return 2;
  }
  if (a == TLS_SCHEMA_V3) {
    return 3;
  }
  if (a == TLS_SCHEMA_V4) {
    return 4;
  }
  return -1;
}

int renew_tl_config(const char *name) {
  if (verbosity >= 2) {
    fprintf(stderr, "Starting config renew\n");
  }
  tl_parse_init();
  auto x = static_cast<unsigned int>(tl_parse_int());
  if (verbosity >= 2) {
    fprintf(stderr, "Schema 0x%08x\n", x);
  }
  schema_version = get_schema_version(x);
  if (verbosity >= 2) {
    fprintf(stderr, "Schema version %d\n", schema_version);
  }
  if (schema_version <= 0 || tl_parse_error()) {
    return -1;
  }

  int new_tl_config_version = tl_parse_int();
  int new_tl_config_date = tl_parse_int();

  if (tl_parse_error()) {
    return -1;
  }

  auto new_crc64 = crc64_partial(inbuf->rptr, ((char *)inbuf->wptr) - (char *)inbuf->rptr, -1ll);
  if (new_tl_config_version < tl_config_version || (new_tl_config_version == tl_config_version && new_tl_config_date < tl_config_date) || config_crc64 == new_crc64) {
    return 0;
  }

  tl_config_alloc();
//  int x;

//  struct tl_type *tps [10000];
  int tn = 0;
//  struct tl_combinator *fns [10000];
  int fn = 0;
  int cn;

  tn = tl_parse_int();
  if (tn < 0 || tn > 10000 || tl_parse_error()) {
    tl_config_back();
    return -1;
  }

  cur_config->tps = reinterpret_cast<tl_type **>(zzmalloc0(sizeof(tl_type *) * tn));
  cur_config->tn = tn;
  ADD_PMALLOC (tn * sizeof(void *));
  struct tl_type **tps = cur_config->tps;
  if (verbosity >= 2) {
    fprintf(stderr, "Found %d types\n", tn);
  }

  for (int i = 0; i < tn; i++) {
    if (tl_parse_int() != TLS_TYPE) {
      tl_config_back();
      return -1;
    }
    tps[i] = read_types();
    if (!tps[i]) {
      tl_config_back();
      return -1;
    }
  }

  cn = tl_parse_int();
  if (cn < 0 || tl_parse_error()) {
    tl_config_back();
    return -1;
  }
  cur_config->cn = cn;

  if (verbosity >= 2) {
    fprintf(stderr, "Found %d constructors\n", cn);
  }

  for (int i = 0; i < cn; i++) {
    if (static_cast<unsigned int>(tl_parse_int()) != (schema_version >= 4 ? TLS_COMBINATOR_V4 : TLS_COMBINATOR)) {
      tl_config_back();
      return -1;
    }
    if (!read_combinators(2)) {
      tl_config_back();
      return -1;
    }
  }
  for (int i = 0; i < tn; ++i) {
    if (!check_type(tps[i])) {
      return -1;
    }
  }
  fn = tl_parse_int();
  if (fn < 0 || fn > 10000 || tl_parse_error()) {
    tl_config_back();
    return -1;
  }
  cur_config->fn = fn;
  cur_config->fns = reinterpret_cast<tl_combinator **>(zzmalloc0(sizeof(tl_combinator *) * fn));
  ADD_PMALLOC (fn * sizeof(void *));
  struct tl_combinator **fns = cur_config->fns;

  if (verbosity >= 2) {
    fprintf(stderr, "Found %d functions\n", fn);
  }

  for (int i = 0; i < fn; i++) {
    if (static_cast<unsigned int>(tl_parse_int()) != (schema_version >= 4 ? TLS_COMBINATOR_V4 : TLS_COMBINATOR)) {
      tl_config_back();
      return -1;
    }
    fns[i] = read_combinators(3);
    if (!fns[i]) {
      tl_config_back();
      return -1;
    }
  }
  tl_parse_end();
  if (tl_parse_error()) {
    tl_config_back();
    return -1;
  }
  static void *IP[10000];
  if (gen_function_fetch(IP, 100) < 0) {
    return -2;
  }
  for (int i = 0; i < tn; i++) {
    if (tps[i]->extra < tps[i]->constructors_num) {
      tl_config_back();
      return -1;
    }
  }
  for (int i = 0; i < tn; i++) {
    for (int j = 0; j < tps[i]->constructors_num; j++) {
      if (gen_constructor_store(tps[i]->constructors[j], IP, 10000) < 0) {
        return -2;
      }
      if (gen_constructor_fetch(tps[i]->constructors[j], IP, 10000) < 0) {
        return -2;
      }
    }
  }
  for (int i = 0; i < fn; i++) {
    if (gen_function_store(fns[i], IP, 10000) < 0) {
      return -2;
    }
  }

  if (name != tl_config_name) {
    if (tl_config_name) {
      ADD_PFREE (strlen(tl_config_name));
      zzstrfree(tl_config_name);
      tl_config_name = NULL;
    }

    tl_config_name = zzstrdup(name);
    ADD_PMALLOC (strlen(name));
  }

  config_crc64 = new_crc64;
  tl_config_version = new_tl_config_version;
  tl_config_date = new_tl_config_date;
  return 1;
}

#define MAX_TL_CONFIG_SIZE (1 << 20)


static int read_tl_config_open(const char *name, struct stat *stat) {
  int fd = open(name, O_RDONLY);

  if (fd < 0) {
    fprintf(stderr, "Can't open .tlo file '%s' (%s)\n", name, strerror(errno));
    return -1;
  }

  if (fstat(fd, stat) < 0) {
    fprintf(stderr, "Can't get stat file '%s' (%s)\n", name, strerror(errno));

    close(fd);
    return -1;
  }

  tl_config_mtime = stat->st_mtime;

  if (stat->st_size == 0) {
    fprintf(stderr, "Empty file '%s'\n", name);

    close(fd);
    return -1;
  }

  if (stat->st_size > MAX_TL_CONFIG_SIZE) {
    fprintf(stderr, "Too big .tlo file '%s' (more than %lld)\n", name, (long long)MAX_TL_CONFIG_SIZE);

    close(fd);
    return -1;
  }

  return fd;
}

static int read_tl_config_process(const char *name, int fd, struct stat *stat) {
  if (verbosity >= 2) {
    fprintf(stderr, "File found. Name %s. size = %lld\n", name, (long long) stat->st_size);
  }

  char *s = static_cast<char *>(malloc(stat->st_size));
  assert (lseek(fd, 0, SEEK_SET) == 0);
  assert (read(fd, s, stat->st_size) == stat->st_size);
  close(fd);
  do_rpc_parse(s, stat->st_size);
  int res = renew_tl_config(name);
  free(s);

  if (res < 0) {
    fprintf(stderr, "Can't reload .tlo file '%s' renew_tl_config returns %d\n", name, res);
  }

  return res;
}

int check_reload_tl_config() {
  if (tl_config_name == NULL || tl_config_mtime == 0) {
    return -1;
  }

  if (!VK_INI_BOOL("tl.conffile_autoreload")) {
    return -1;
  }

  time_t current_mtime = tl_config_mtime;
  struct stat stat;

  int fd = read_tl_config_open(tl_config_name, &stat);
  if (fd < 0) {
    return -1;
  }

  if (current_mtime < stat.st_mtime) {
    return read_tl_config_process(tl_config_name, fd, &stat);
  }

  close(fd);
  return -1;
}

int read_tl_config(const char *name) {
  struct stat stat;

  int fd = read_tl_config_open(name, &stat);
  if (fd < 0) {
    return -1;
  }

  return read_tl_config_process(name, fd, &stat);
}

/* }}} */
