// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VKEXT_SCHEMA_MEMCACHE_H__
#define __VKEXT_SCHEMA_MEMCACHE_H__

#include "vkext/vk_zend.h"

long long vk_queries_count(INTERNAL_FUNCTION_PARAMETERS);
void vk_rpc_tl_query(INTERNAL_FUNCTION_PARAMETERS);
void vk_rpc_tl_query_one(INTERNAL_FUNCTION_PARAMETERS);
void vk_rpc_tl_query_result(INTERNAL_FUNCTION_PARAMETERS);
void vk_rpc_tl_query_result_one(INTERNAL_FUNCTION_PARAMETERS);
extern int typed_mode;
extern const char *tl_current_function_name;

struct tl_type {
//  struct tl_type_methods *methods;
  char *id;
  unsigned name;
  int arity;
  int flags;
  int constructors_num;
  struct tl_combinator **constructors;
  long long params_types;
  int extra;
};

struct tl_type_methods {
  struct tl_tree *(*store)(struct tl_combinator *t, zval **arr, struct tl_tree **vars);
  zval **(*fetch)(struct tl_type *t, struct tl_tree *res);
};

struct tl_combinator_methods {
  struct tl_tree *(*store)(struct tl_combinator *c, zval **arr, struct tl_tree *tree);
  zval **(*fetch)(struct tl_combinator *c, struct tl_tree *res);
  int (*check)(struct tl_combinator *c, struct tl_tree *res);
};

void tl_parse_on_rinit();
void tl_parse_on_rshutdown();
#define NODE_TYPE_TYPE 1
#define NODE_TYPE_NAT_CONST 2
#define NODE_TYPE_VAR_TYPE 3
#define NODE_TYPE_VAR_NUM 4
#define NODE_TYPE_ARRAY 5

#define MAX_COMBINATOR_VARS 64

#define NAME_VAR_NUM 0x70659eff
#define NAME_VAR_TYPE 0x2cecf817
#define NAME_INT 0xa8509bda
#define NAME_LONG 0x22076cba
#define NAME_DOUBLE 0x2210c154
#define NAME_FLOAT 0x824dab22U
#define NAME_STRING 0xb5286e24
#define NAME_VECTOR 0x1cb5c415
#define NAME_DICTIONARY 0x1f4c618f
#define NAME_INT_KEY_DICTIONARY 0x07bafc42
#define NAME_LONG_KEY_DICTIONARY 0xb424d8f1
#define NAME_MAYBE_TRUE 0x3f9c8ef8
#define NAME_MAYBE_FALSE 0x27930a7b
#define NAME_BOOL_FALSE 0xbc799737
#define NAME_BOOL_TRUE 0x997275b5

#define TYPE_NAME_BOOL 0x250be282
#define TYPE_NAME_MAYBE 0x180f8483

#define FLAG_OPT_VAR (1 << 17)
#define FLAG_EXCL (1 << 18)
#define FLAG_OPT_FIELD (1 << 20)
#define FLAG_NOVAR (1 << 21)
#define FLAG_BARE 1
#define FLAGS_MASK ((1 << 16) - 1)
#define FLAG_DEFAULT_CONSTRUCTOR (1 << 25)
#define FLAG_NOCONS (1 << 1)
#define FLAG_FORWARDED (1 << 27)

#define COMBINATOR_FLAG_READ 1
#define COMBINATOR_FLAG_WRITE 2
#define COMBINATOR_FLAG_INTERNAL 4
#define COMBINATOR_FLAG_KPHP 8

extern struct tl_tree_methods tl_nat_const_methods;
extern struct tl_tree_methods tl_nat_const_full_methods;
extern struct tl_tree_methods tl_pnat_const_full_methods;
extern struct tl_tree_methods tl_array_methods;
extern struct tl_tree_methods tl_type_methods;
extern struct tl_tree_methods tl_parray_methods;
extern struct tl_tree_methods tl_ptype_methods;
extern struct tl_tree_methods tl_var_num_methods;
extern struct tl_tree_methods tl_var_type_methods;
extern struct tl_tree_methods tl_pvar_num_methods;
extern struct tl_tree_methods tl_pvar_type_methods;
#define TL_IS_NAT_VAR(x) (((long)x) & 1)
#define TL_TREE_METHODS(x) (TL_IS_NAT_VAR (x) ? &tl_nat_const_methods : ((struct tl_tree *)(x))->methods)

#define DEC_REF(x) (TL_TREE_METHODS(x)->dec_ref (reinterpret_cast<tl_tree *>(x)))
#define INC_REF(x) (TL_TREE_METHODS(x)->inc_ref (reinterpret_cast<tl_tree *>(x)))
#define TYPE(x) (TL_TREE_METHODS(x)->type (reinterpret_cast<tl_tree *>(x)))

typedef unsigned long long tl_tree_hash_t;

struct tl_tree_methods {
  int (*type)(struct tl_tree *T);
  int (*eq)(struct tl_tree *T, struct tl_tree *U);
  void (*inc_ref)(struct tl_tree *T);
  void (*dec_ref)(struct tl_tree *T);
};

struct tl_tree {
  int ref_cnt;
  int flags;
  //tl_tree_hash_t hash;
  struct tl_tree_methods *methods;
  const char *forwarded_fun_name;
};
/*
struct tl_tree_nat_const {
  struct tl_tree self;
  int value;
};*/

struct tl_tree_type {
  struct tl_tree self;

  struct tl_type *type;
  int children_num;
  struct tl_tree **children;
};

struct tl_tree_array {
  struct tl_tree self;

  struct tl_tree *multiplicity;
  int args_num;
  struct arg **args;
};

struct tl_tree_var_type {
  struct tl_tree self;

  int var_num;
};

struct tl_tree_var_num {
  struct tl_tree self;

  int var_num;
  int dif;
};

struct tl_tree_nat_const {
  struct tl_tree self;

  long long value;
};

struct arg {
  char *id;
  int var_num;
  int flags;
  int exist_var_num;
  int exist_var_bit;
  struct tl_tree *type;
};

struct tl_combinator {
  //struct tl_combinator_methods *methods;
  char *id;
  unsigned name;
  int is_fun;
  int var_num;
  int flags;
  int args_num;
  struct arg **args;
  struct tl_tree *result;
  void **IP;
  void **fIP;
  int IP_len;
  int fIP_len;
};

int renew_tl_config(const char *name);
int read_tl_config(const char *name);
void tl_delete_old_configs();
struct tl_combinator *tl_fun_get_by_id(const char *s);
int check_reload_tl_config();
#endif
