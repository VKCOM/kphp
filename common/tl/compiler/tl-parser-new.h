// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __TL_PARSER_NEW_H__
#define __TL_PARSER_NEW_H__
enum lex_type { lex_error, lex_char, lex_triple_minus, lex_uc_ident, lex_lc_ident, lex_eof, lex_none, lex_num, lex_attribute };

struct curlex {
  char* ptr;
  int len;
  enum lex_type type;
  int flags;
};

struct parse {
  char* filename;
  int filename_len;
  char* text;
  int pos;
  int len;
  int line;
  int line_pos;
  struct curlex lex;
};

enum tree_type {
  type_tl_program,
  type_fun_declarations,
  type_constr_declarations,
  type_declaration,
  type_combinator_decl,
  type_equals,
  type_full_combinator_id,
  type_opt_args,
  type_args,
  type_args1,
  type_args2,
  type_args3,
  type_args4,
  type_boxed_type_ident,
  type_subexpr,
  type_var_ident,
  type_var_ident_opt,
  type_multiplicity,
  type_type_term,
  type_term,
  type_percent,
  type_result_type,
  type_expr,
  type_nat_term,
  type_combinator_id,
  type_nat_const,
  type_type_ident,
  type_builtin_combinator_decl,
  type_exclam,
  type_optional_arg_def,
  type_attribute,
  type_tl_schemas
};

struct tree {
  char* text;
  int len;
  enum tree_type type;
  int lex_line;
  int lex_line_pos;
  int flags;
  int size;
  int nc;
  struct tree** c;
  char* filename;
  int filename_len;
};

#define TL_ACT(x)                                                                                                                                              \
  (x == act_var             ? "act_var"                                                                                                                        \
   : x == act_field         ? "act_field"                                                                                                                      \
   : x == act_plus          ? "act_plus"                                                                                                                       \
   : x == act_type          ? "act_type"                                                                                                                       \
   : x == act_nat_const     ? "act_nat_const"                                                                                                                  \
   : x == act_array         ? "act_array"                                                                                                                      \
   : x == act_question_mark ? "act_question_mark"                                                                                                              \
   : x == act_union         ? "act_union"                                                                                                                      \
   : x == act_arg           ? "act_arg"                                                                                                                        \
   : x == act_opt_field     ? "act_opt_field"                                                                                                                  \
                            : "act_unknown")

#define TL_TYPE(x)                                                                                                                                             \
  (x == type_num         ? "type_num"                                                                                                                          \
   : x == type_type      ? "type_type"                                                                                                                         \
   : x == type_list_item ? "type_list_item"                                                                                                                    \
   : x == type_list      ? "type_list"                                                                                                                         \
   : x == type_num_value ? "type_num_value"                                                                                                                    \
                         : "type_unknown")
enum combinator_tree_action { act_var, act_field, act_plus, act_type, act_nat_const, act_array, act_question_mark, act_union, act_arg, act_opt_field };

enum combinator_tree_type { type_num, type_num_value, type_type, type_list_item, type_list };

struct tl_combinator_tree {
  enum combinator_tree_action act;
  struct tl_combinator_tree *left, *right;
  char* name;
  void* data;
  long long flags;
  enum combinator_tree_type type;
  int type_len;
  long long type_flags;
};

struct tl_program {
  int types_num;
  int functions_num;
  int constructors_num;
  struct tl_type** types;
  struct tl_function** functions;
  //  struct tl_constuctor **constructors;
};

struct tl_type {
  char* id;
  char* print_id;
  char* real_id;
  unsigned name;
  int flags;

  int params_num;
  long long params_types;

  int constructors_num;
  struct tl_constructor** constructors;
};

struct tl_constructor {
  char* id;
  char* print_id;
  char* real_id;
  unsigned name;
  unsigned int read : 1;
  unsigned int write : 1;
  unsigned int internal : 1;
  unsigned int kphp : 1;
  unsigned int is_function : 1;
  struct tl_type* type;

  struct tl_combinator_tree* left;
  struct tl_combinator_tree* right;
  char* filename;
  int filename_len;
};

struct tl_var {
  char* id;
  struct tl_combinator_tree* ptr;
  int type;
  int flags;
};

int tl_init_parse_file(const char* fname, struct parse* save);
struct tree* tl_parse_lex(struct parse* parse_files, int files_count);
void tl_print_parse_error();
struct tl_program* tl_parse(struct tree* T);

void write_types(int f);

#define FLAG_BARE 1
#define FLAG_OPT_VAR (1 << 17)
#define FLAG_EXCL (1 << 18)
#define FLAG_OPT_FIELD (1 << 20)
#define FLAG_IS_VAR (1 << 21)
#define FLAG_DEFAULT_CONSTRUCTOR (1 << 25)
#define FLAG_EMPTY (1 << 10)

#define COMBINATOR_FLAG_READ 1
#define COMBINATOR_FLAG_WRITE 2
#define COMBINATOR_FLAG_INTERNAL 4
#define COMBINATOR_FLAG_KPHP 8

#endif
