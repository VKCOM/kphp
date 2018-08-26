#include "compiler/gentree.h"

#include <sstream>

#include "common/algorithms/find.h"

#include "compiler/compiler-core.h"
#include "compiler/debug.h"
#include "compiler/io.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/stage.h"

GenTree::GenTree ()
  : line_num(-1)
  , tokens(NULL)
  , callback(NULL)
  , in_func_cnt_(0)
  , is_top_of_the_function_(false)
{}

#define CE(x) if (!(x)) {return VertexPtr();}

void GenTree::init (const vector <Token *> *tokens_new, const string &context, GenTreeCallbackBase *callback_new) {
  line_num = 0;
  in_func_cnt_ = 0;
  tokens = tokens_new;
  callback = callback_new;
  class_stack = vector <ClassInfo>();

  cur = tokens->begin();
  end = tokens->end();

  namespace_name = "";
  class_context = context;
  if (!class_context.empty()) {
    context_class_ptr = callback_new->get_class_by_name(class_context);
    if (context_class_ptr.is_null()) { // Sometimes fails, debug output
      fprintf(stderr, "context class = %s\n", class_context.c_str());
    }
    kphp_assert(context_class_ptr.not_null());
  } else {
    context_class_ptr = ClassPtr();
  }

  kphp_assert (cur != end);
  end--;
  kphp_assert ((*end)->type() == tok_end);

  line_num = (*cur)->line_num;
  stage::set_line (line_num);
}

static string replace_backslashes(const string &s) {
  return replace_characters(s, '\\', '$');
}

bool GenTree::in_class() {
  return !class_stack.empty();
}
bool GenTree::in_namespace() const {
  return !namespace_name.empty();
}

ClassInfo &GenTree::cur_class() {
  kphp_assert (in_class());
  return class_stack.back();
}

FunctionPtr GenTree::register_function (FunctionInfo info) {
  stage::set_line(0);
  info.root = post_process(info.root);

  if (in_class() && !in_namespace()) {
    return FunctionPtr();
  }

  FunctionPtr function_ptr = callback->register_function(info);

  if (!in_class()) {
    return function_ptr;
  }

  if (function_ptr->type() != FunctionData::func_global) {
    const string &name = function_ptr->name;
    size_t first = name.find("$$") + 2;
    kphp_assert(first != string::npos);
    size_t second = name.find("$$", first);
    if (second != string::npos) {
      second = name.size();
    }

    if (function_ptr->is_instance_function()) {
      cur_class().methods.push_back(function_ptr->root);
    } else {
      cur_class().static_methods[name.substr(first, second - first)] = function_ptr;
    }

    if (function_ptr->is_instance_function()) {
      callback->require_function_set(function_ptr);
    }
  }
  return function_ptr;
}

void GenTree::enter_class (const string &class_name, Token *phpdoc_token) {
  class_stack.push_back(ClassInfo());
  cur_class().name = class_name;
  cur_class().phpdoc_token = phpdoc_token;
  if (in_namespace()) {
    cur_class().namespace_name = namespace_name;
    kphp_error(class_stack.size() <= 1, "Nested classes are not supported");
  }
}

VertexPtr GenTree::generate_constant_field_class(VertexPtr root) {
  CREATE_VERTEX (name_of_const_field_class, op_func_name);
  name_of_const_field_class->str_val = "c#" + replace_backslashes(namespace_name) + "$" + cur_class().name + "$$class";

  kphp_error_act(cur_class().constants.find("class") == cur_class().constants.end(),
                    ("A class constant must not be called 'class'; it is reserved for class name fetching: " + name_of_const_field_class->str_val).c_str(), return VertexPtr{});

  CREATE_VERTEX(value_of_const_field_class, op_string);
  value_of_const_field_class->set_string(namespace_name + "\\" + cur_class().name);

  CREATE_VERTEX(def, op_define, name_of_const_field_class, value_of_const_field_class);
  def->location = root->location;

  return def;
}

void GenTree::exit_and_register_class (VertexPtr root) {
  kphp_assert (in_class());
  if (!in_namespace()) {
    kphp_error(false, "Only classes in namespaces are supported");
  } else {

    VertexPtr constant_field_class = generate_constant_field_class(root);
    if (constant_field_class.not_null()) {
      cur_class().constants["class"] = constant_field_class;
    }

    const string &name_str = stage::get_file()->main_func_name;
    vector<VertexPtr> empty;
    CREATE_VERTEX (name, op_func_name);
    name->str_val = name_str;
    CREATE_VERTEX (params, op_func_param_list, empty);

    vector <VertexPtr> seq = get_map_values(cur_class().constants);
    seq.insert(seq.end(), cur_class().static_members.begin(), cur_class().static_members.end());

    CREATE_VERTEX (func_root, op_seq, seq);
    CREATE_VERTEX (main, op_function, name, params, func_root);
    func_force_return(main);

    main->auto_flag = false;
    main->varg_flag = false;
    main->throws_flag = false;
    main->resumable_flag = false;
    main->extra_type = op_ex_func_global;

    const FunctionInfo &info = FunctionInfo(main, namespace_name, cur_class().name, class_context,
                                            this->namespace_uses, class_extends, false, access_nonmember);
    kphp_assert(register_function(info).not_null());
  }
  cur_class().root = root;
  cur_class().extends = class_extends;
  if ((cur_class().has_instance_vars() || cur_class().has_instance_methods()) && cur_class().new_function.is_null()) {
    cur_class().new_function = create_default_constructor(cur_class());
  }
  if (namespace_name + "\\" + cur_class().name == class_context) {
    kphp_assert(callback->register_class(cur_class()).not_null());
  }
  class_stack.pop_back();
}

void GenTree::enter_function() {
  in_func_cnt_++;
}
void GenTree::exit_function() {
  in_func_cnt_--;
}

void GenTree::next_cur (void) {
  if (cur != end) {
    cur++;
    if ((*cur)->line_num != -1) {
      line_num = (*cur)->line_num;
      stage::set_line (line_num);
    }
  }
}

bool GenTree::test_expect (TokenType tp) {
  return (*cur)->type() == tp;
}

#define expect(tp, msg) ({ \
  bool res;\
  if (kphp_error (test_expect (tp), dl_pstr ("Expected %s, found '%s'", msg, cur == end ? "END OF FILE" : (*cur)->to_str().c_str()))) {\
    res = false;\
  } else {\
    next_cur();\
    res = true;\
  }\
  res; \
})

#define expect2(tp1, tp2, msg) ({ \
  kphp_error (test_expect (tp1) || test_expect (tp2), dl_pstr ("Expected %s, found '%s'", msg, cur == end ? "END OF FILE" : (*cur)->to_str().c_str())); \
  if (cur != end) {next_cur();} \
  1;\
})

VertexPtr GenTree::get_var_name() {
  AutoLocation var_location (this);

  if ((*cur)->type() != tok_var_name) {
    return VertexPtr();
  }
  CREATE_VERTEX (var, op_var);
  var->str_val = (*cur)->str_val;

  set_location (var, var_location);

  next_cur();
  return var;
}

VertexPtr GenTree::get_var_name_ref() {
  int ref_flag = 0;
  if ((*cur)->type() == tok_and) {
    next_cur();
    ref_flag = 1;
  }

  VertexPtr name = get_var_name();
  if (name.not_null()) {
    name->ref_flag = ref_flag;
  } else {
    kphp_error (ref_flag == 0, "Expected var name");
  }
  return name;
}

int GenTree::open_parent() {
  if ((*cur)->type() == tok_oppar) {
    next_cur();
    return 1;
  }
  return 0;
}

inline void GenTree::skip_phpdoc_tokens () {
  while ((*cur)->type() == tok_phpdoc) {
    next_cur();
  }
}

#define close_parent(is_opened)\
  if (is_opened) {\
    CE (expect (tok_clpar, "')'"));\
  }

template <Operation EmptyOp>
bool GenTree::gen_list (vector <VertexPtr> *res, GetFunc f, TokenType delim) {
  //Do not clear res. Result must be appended to it.
  bool prev_delim = false;
  bool next_delim = true;

  while (next_delim) {
    VertexPtr v = (this->*f)();
    next_delim = (*cur)->type() == delim;

    if (v.is_null()) {
      if (EmptyOp != op_err && (prev_delim || next_delim)) {
        if (EmptyOp == op_none) {
          break;
        }
        CREATE_VERTEX (tmp, EmptyOp);
        v = tmp;
      } else if (prev_delim) {
        kphp_error (0, "Expected something after ','");
        return false;
      } else {
        break;
      }
    }

    res->push_back (v);
    prev_delim = true;

    if (next_delim) {
      next_cur();
    }
  }

  return true;
}

template <Operation Op> VertexPtr GenTree::get_conv() {
  AutoLocation conv_location (this);
  next_cur();
  VertexPtr first_node = get_expression();
  CE (!kphp_error (first_node.not_null(), "get_conv failed"));
  CREATE_VERTEX (conv, Op, first_node);
  set_location (conv, conv_location);
  return conv;
}

template <Operation Op> VertexPtr GenTree::get_varg_call() {
  AutoLocation call_location (this);
  next_cur();

  CE (expect (tok_oppar, "'('"));

  AutoLocation args_location (this);
  vector <VertexPtr> args_next;
  bool ok_args_next = gen_list <op_err> (&args_next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error (ok_args_next, "get_varg_call failed"));
  CREATE_VERTEX (args, op_array, args_next);
  set_location (args, args_location);

  CE (expect (tok_clpar, "')'"));

  CREATE_VERTEX (call, Op, args);
  set_location (call, call_location);
  return call;
}

template <Operation Op> VertexPtr GenTree::get_require() {
  AutoLocation require_location (this);
  next_cur();
  bool is_opened = open_parent();
  vector <VertexPtr> next;
  bool ok_next = gen_list <op_err> (&next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error (ok_next, "get_require_list failed"));
  close_parent (is_opened);
  CREATE_VERTEX (require, Op, next);
  set_location (require, require_location);
  return require;
}
template <Operation Op, Operation EmptyOp> VertexPtr GenTree::get_func_call() {
  AutoLocation call_location (this);
  string name = (*cur)->str_val;
  next_cur();

  CE (expect (tok_oppar, "'('"));
  skip_phpdoc_tokens();
  vector <VertexPtr> next;
  bool ok_next = gen_list<EmptyOp> (&next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error (ok_next, "get argument list failed"));
  CE (expect (tok_clpar, "')'"));

  if (Op == op_isset) {
    CE (!kphp_error (!next.empty(), "isset function requires at least one argument"));
    CREATE_VERTEX (left, op_isset, next[0]);
    for (size_t i = 1; i < next.size(); i++) {
      CREATE_VERTEX (right, op_isset, next[i]);
      CREATE_VERTEX (log_and, op_log_and, left, right);
      left = log_and;
    }
    set_location (left, call_location);
    return left;
  }

  CREATE_VERTEX (call, Op, next);
  set_location (call, call_location);

  //hack..
  if (Op == op_func_call) {
    VertexAdaptor <op_func_call> func_call = call;
    func_call->set_string(name);
  }
  if (Op == op_constructor_call) {
    VertexAdaptor <op_constructor_call> func_call = call;
    func_call->set_string(name);

    // todo optimize
    if (name.size() == 8 && name == "Memcache") {
      func_call->type_help = tp_MC;
    }
    if (name == "true_mc" || name == "test_mc" || name == "RpcMemcache") {
      func_call->type_help = tp_MC;
    }
    if (name.size() == 9 && name == "Exception") {
      func_call->type_help = tp_Exception;
    }
    if (name.size() == 10 && name == "\\Exception") {
      func_call->set_string("Exception");
      func_call->type_help = tp_Exception;
    }
  }
  return call;
}
VertexPtr GenTree::get_short_array() {
  AutoLocation call_location (this);
  next_cur();

  vector <VertexPtr> next;
  bool ok_next = gen_list<op_none> (&next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error (ok_next, "get short array failed"));
  CE (expect (tok_clbrk, "']'"));

  CREATE_VERTEX (arr, op_array, next);
  set_location (arr, call_location);

  return arr;
}


VertexPtr GenTree::get_string() {
  CREATE_VERTEX (str, op_string);
  set_location (str, AutoLocation(this));
  str->str_val = (*cur)->str_val;
  next_cur();
  return str;
}

VertexPtr GenTree::get_string_build() {
  AutoLocation sb_location (this);
  vector <VertexPtr> v_next;
  next_cur();
  bool after_simple_expression = false;
  while (cur != end && (*cur)->type() != tok_str_end) {
    if ((*cur)->type() == tok_str) {
      v_next.push_back (get_string());
      if (after_simple_expression) {
        VertexAdaptor<op_string> last = v_next.back().as<op_string>();
        if (last->str_val != "" && last->str_val[0] == '[') {
          kphp_warning("Simple string expressions with [] can work wrong. Use more {}");
        }
      }
      after_simple_expression = false;
    } else if ((*cur)->type() == tok_expr_begin) {
      next_cur();

      VertexPtr add = get_expression();
      CE (!kphp_error (add.not_null(), "Bad expression in string"));
      v_next.push_back (add);

      CE (expect (tok_expr_end, "'}'"));
      after_simple_expression = false;
    } else {
      after_simple_expression = true;
      VertexPtr add = get_expression();
      CE (!kphp_error (add.not_null(), "Bad expression in string"));
      v_next.push_back (add);
    }
  }
  CE (expect (tok_str_end, "'\"'"));
  CREATE_VERTEX (sb, op_string_build, v_next);
  set_location (sb, sb_location);
  return sb;
}

VertexPtr GenTree::get_postfix_expression (VertexPtr res) {
  //postfix operators ++, --, [], ->
  bool need = true;
  while (need && cur != end) {
    vector <Token*>::const_iterator op = cur;
    TokenType tp = (*op)->type();
    need = false;

    if (tp == tok_inc) {
      CREATE_VERTEX (v, op_postfix_inc, res);
      set_location (v, AutoLocation(this));
      res = v;
      need = true;
      next_cur();
    } else if (tp == tok_dec) {
      CREATE_VERTEX (v, op_postfix_dec, res);
      set_location (v, AutoLocation(this));
      res = v;
      need = true;
      next_cur();
    } else if (tp == tok_opbrk || tp == tok_opbrc) {
      AutoLocation location (this);
      next_cur();
      VertexPtr i = get_expression();
      if (tp == tok_opbrk) {
        CE (expect (tok_clbrk, "']'"));
      } else {
        CE (expect (tok_clbrc, "'}'"));
      }
      //TODO: it should be to separate operations
      if (i.is_null()) {
        CREATE_VERTEX (v, op_index, res);
        res = v;
      } else {
        CREATE_VERTEX (v, op_index, res, i);
        res = v;
      }
      set_location (res, location);
      need = true;
    } else if (tp == tok_arrow) {
      AutoLocation location (this);
      next_cur();
      VertexPtr i = get_expr_top();
      CE (!kphp_error (i.not_null(), "Failed to parse right argument of '->'"));
      CREATE_VERTEX (v, op_arrow, res, i);
      res = v;
      set_location (res, location);
      need = true;
    }

  }
  return res;
}

VertexPtr GenTree::get_expr_top() {
  vector <Token*>::const_iterator op = cur;

  VertexPtr res, first_node;
  TokenType type = (*op)->type();

  bool return_flag = true;
  switch (type) {
    case tok_line_c: {
      CREATE_VERTEX (v, op_int_const);
      set_location (v, AutoLocation(this));
      v->str_val = int_to_str (stage::get_line());
      res = v;
      next_cur();
      break;
    }
    case tok_file_c: {
      CREATE_VERTEX (v, op_string);
      set_location (v, AutoLocation(this));
      v->str_val = stage::get_file()->file_name;
      next_cur();
      res = v;
      break;
    }
    case tok_func_c: {
      CREATE_VERTEX (v, op_function_c);
      set_location (v, AutoLocation(this));
      next_cur();
      res = v;
      break;
    }
    case tok_int_const: {
      CREATE_VERTEX (v, op_int_const);
      set_location (v, AutoLocation(this));
      v->str_val = (*cur)->str_val;
      next_cur();
      res = v;
      break;
    }
    case tok_float_const: {
      CREATE_VERTEX (v, op_float_const);
      set_location (v, AutoLocation(this));
      v->str_val = (*cur)->str_val;
      next_cur();
      res = v;
      break;
    }
    case tok_null: {
      CREATE_VERTEX (v, op_null);
      set_location (v, AutoLocation(this));
      next_cur();
      res = v;
      break;
    }
    case tok_false: {
      CREATE_VERTEX (v, op_false);
      set_location (v, AutoLocation(this));
      next_cur();
      res = v;
      break;
    }
    case tok_true: {
      CREATE_VERTEX (v, op_true);
      set_location (v, AutoLocation(this));
      next_cur();
      res = v;
      break;
    }
    case tok_var_name: {
      res = get_var_name();
      return_flag = false;
      break;
    }
    case tok_str:
      res = get_string();
      break;
    case tok_conv_int:
      res = get_conv <op_conv_int>();
      break;
    case tok_conv_bool:
      res = get_conv <op_conv_bool>();
      break;
    case tok_conv_float:
      res = get_conv <op_conv_float>();
      break;
    case tok_conv_string:
      res = get_conv <op_conv_string>();
      break;
    case tok_conv_long:
      res = get_conv <op_conv_long>();
      break;
    case tok_conv_uint:
      res = get_conv <op_conv_uint>();
      break;
    case tok_conv_ulong:
      res = get_conv <op_conv_ulong>();
      break;
    case tok_conv_array:
      res = get_conv <op_conv_array>();
      break;

    case tok_print: {
      AutoLocation print_location (this);
      next_cur();
      first_node = get_expression();
      CE (!kphp_error (first_node.not_null(), "Failed to get print argument"));
      first_node = conv_to <tp_string> (first_node);
      CREATE_VERTEX (print, op_print, first_node);
      set_location (print, print_location);
      res = print;
      break;
    }

    case tok_exit:
      res = get_exit();
      break;
    case tok_require:
      res = get_require <op_require>();
      break;
    case tok_require_once:
      res = get_require <op_require_once>();
      break;

    case tok_constructor_call:
      res = get_func_call <op_constructor_call, op_none>();
      break;
    case tok_func_name: {
      bool was_arrow = (*(cur - 1))->type() == tok_arrow;
      cur++;
      if (!test_expect (tok_oppar)) {
        cur--;
        CREATE_VERTEX (v, op_func_name);
        set_location (v, AutoLocation(this));
        next_cur();
        v->str_val = (*op)->str_val;
        res = v;
        return_flag = was_arrow;
        break;
      }
      cur--;
      res = get_func_call <op_func_call, op_err>();
      return_flag = was_arrow;
      break;
    }
    case tok_function:
      res = get_function (true);
      break;
    case tok_isset:
      res = get_func_call <op_isset, op_err>();
      break;
    case tok_array:
      res = get_func_call <op_array, op_none>();
      break;
    case tok_opbrk:
      res = get_short_array();
      break;
    case tok_list:
      res = get_func_call <op_list_ce, op_lvalue_null>();
      break;
    case tok_defined:
      res = get_func_call <op_defined, op_err>();
      break;
    case tok_min: {
      VertexAdaptor <op_min> min_v = get_func_call <op_min, op_err>();
      VertexRange args = min_v->args();
      if (args.size() == 1) {
        args[0] = GenTree::conv_to (args[0], tp_array);
      }
      res = min_v;
      break;
    }
    case tok_max: {
      VertexAdaptor <op_max> max_v = get_func_call <op_max, op_err>();
      VertexRange args = max_v->args();
      if (args.size() == 1) {
        args[0] = GenTree::conv_to (args[0], tp_array);
      }
      res = max_v;
      break;
    }

    case tok_oppar:
      next_cur();
      res = get_expression();
      CE (!kphp_error (res.not_null(), "Failed to parse expression after '('"));
      res->parent_flag = true;
      CE (expect (tok_clpar, "')'"));
      return_flag = (*cur)->type() != tok_arrow;
      break;
    case tok_str_begin:
      res = get_string_build();
      break;
    default:
      return VertexPtr();
  }

  if (return_flag) {
    return res;
  }

  res = get_postfix_expression (res);
  return res;
}

VertexPtr GenTree::get_unary_op() {
  Operation tp = OpInfo::tok_to_unary_op[(*cur)->type()];
  if (tp != op_err) {
    AutoLocation expr_location (this);
    next_cur();

    VertexPtr left = get_unary_op();
    if (left.is_null()) {
      return VertexPtr();
    }

    if (tp == op_log_not) {
      left = conv_to <tp_bool> (left);
    }
    if (tp == op_not) {
      left = conv_to <tp_int> (left);
    }
    CREATE_META_VERTEX_1 (expr, meta_op_unary_op, tp, left);
    set_location (expr, expr_location);
    return expr;
  }

  VertexPtr res = get_expr_top();
  return res;
}


VertexPtr GenTree::get_binary_op (int bin_op_cur, int bin_op_end, GetFunc next, bool till_ternary) {
  if (bin_op_cur == bin_op_end) {
    return (this->*next)();
  }

  VertexPtr left = get_binary_op (bin_op_cur + 1, bin_op_end, next, till_ternary);
  if (left.is_null()) {
    return VertexPtr();
  }

  bool need = true;
  bool ternary = bin_op_cur == OpInfo::ternaryP;
  //fprintf (stderr, "get binary op: [%d..%d], cur = %d[%s]\n", bin_op_cur, bin_op_end, tok_priority[cur == end ? 0 : (*cur)->type()], cur == end ? "<none>" : (*cur)->to_str().c_str());
  while (need && cur != end) {
    Operation tp = OpInfo::tok_to_binary_op[(*cur)->type()];
    if (tp == op_err || OpInfo::priority (tp) != bin_op_cur) {
      break;
    }
    if (ternary && till_ternary) {
      break;
    }
    AutoLocation expr_location (this);

    bool left_to_right = OpInfo::fixity (tp) == left_opp;

    next_cur();
    VertexPtr right, third;
    if (ternary) {
      right = get_expression();
    } else {
      right = get_binary_op (bin_op_cur + left_to_right, bin_op_end, next, till_ternary && bin_op_cur >= OpInfo::ternaryP);
    }
    if (right.is_null() && !ternary) {
      kphp_error (0, dl_pstr ("Failed to parse second argument in [%s]", OpInfo::str (tp).c_str()));
      return VertexPtr();
    }

    if (ternary) {
      CE (expect (tok_colon, "':'"));
      //third = get_binary_op (bin_op_cur + 1, bin_op_end, next);
      third = get_expression_impl (true);
      if (third.is_null()) {
        kphp_error (0, dl_pstr ("Failed to parse third argument in [%s]", OpInfo::str (tp).c_str()));
        return VertexPtr();
      }
      if (!right.is_null())
         left = conv_to <tp_bool> (left);
    }


    if (tp == op_log_or || tp == op_log_and || tp == op_log_or_let || tp == op_log_and_let || tp == op_log_xor_let) {
      left = conv_to <tp_bool> (left);
      right = conv_to <tp_bool> (right);
    }
    if (tp == op_set_or || tp == op_set_and || tp == op_set_xor || tp == op_set_shl || tp == op_set_shr) {
      right = conv_to <tp_int> (right);
    }
    if (tp == op_or || tp == op_and || tp == op_xor) {
      left = conv_to <tp_int> (left);
      right = conv_to <tp_int> (right);
    }

    VertexPtr expr;
    if (ternary) {
      if (!right.is_null()) {
        CREATE_VERTEX (v, op_ternary, left, right, third);
        expr = v;
      } else {
        string left_name = gen_shorthand_ternary_name();
        CREATE_VERTEX (left_var, op_var);
        left_var->str_val = left_name;
        left_var->extra_type = op_ex_var_superlocal;
        CREATE_VERTEX (left_set, op_set, left_var, left);
        left_set = conv_to<tp_bool>(left_set);


        CREATE_VERTEX (left_var_copy, op_var);
        left_var_copy->str_val = left_name;
        left_var_copy->extra_type = op_ex_var_superlocal;

        CREATE_VERTEX(left_var_move, op_move, left_var_copy);

        CREATE_VERTEX (result, op_ternary, left_set, left_var_move, third);

        expr = result;
      }
    } else {
      CREATE_META_VERTEX_2 (v, meta_op_binary_op, tp, left, right);
      expr = v;
    }
    set_location (expr, expr_location);

    left = expr;

    need = need && (left_to_right || ternary);
  }
  return left;

}

VertexPtr GenTree::get_expression_impl (bool till_ternary) {
  return get_binary_op (OpInfo::bin_op_begin, OpInfo::bin_op_end, &GenTree::get_unary_op, till_ternary);
}
VertexPtr GenTree::get_expression() {
  skip_phpdoc_tokens();
  return get_expression_impl (false);
}

VertexPtr GenTree::embrace (VertexPtr v) {
  if (v->type() != op_seq) {
    CREATE_VERTEX (brace, op_seq, v);
    ::set_location (brace, v->get_location());
    return brace;
  }

  return v;
}

VertexPtr GenTree::get_def_value() {
  VertexPtr val;

  if ((*cur)->type() == tok_eq1) {
    next_cur();
    val = get_expression();
    kphp_error (val.not_null(), "Cannot parse function parameter");
  }

  return val;
}

VertexPtr GenTree::get_func_param () {
  VertexPtr res;
  AutoLocation st_location (this);
  if (test_expect(tok_func_name) && (*(cur + 1))->type() == tok_oppar) { // callback
    CREATE_VERTEX (name, op_func_name);
    set_location (name, st_location);
    name->str_val = (*cur)->str_val;
    next_cur();

    CE (expect (tok_oppar, "'('"));
    int param_cnt = 0;
    if (!test_expect (tok_clpar)) {
      while (true) {
        param_cnt++;
        CE (expect (tok_var_name, "'var_name'"));
        if (test_expect (tok_clpar)) {
          break;
        }
        CE (expect (tok_comma, "','"));
      }
    }
    CE (expect (tok_clpar, "')'"));

    vector <VertexPtr> next;
    next.push_back (name);
    VertexPtr def_val = get_def_value();
    if (def_val.not_null()) {
      next.push_back (def_val);
    }
    CREATE_VERTEX (v, op_func_param_callback, next);
    set_location (v, st_location);
    v->param_cnt = param_cnt;
    res = v;
  } else {
    Token *tok_type_declaration = NULL;
    if ((*cur)->type() == tok_func_name || (*cur)->type() == tok_Exception) {
      tok_type_declaration = *cur;
      next_cur();
    }

    VertexPtr name = get_var_name_ref();
    if (name.is_null()) {
      return VertexPtr();
    }

    vector <VertexPtr> next;
    next.push_back (name);

    PrimitiveType tp = get_type_help();

    VertexPtr def_val = get_def_value();
    if (def_val.not_null()) {
      next.push_back (def_val);
    }
    CREATE_VERTEX (v, op_func_param, next);
    set_location (v, st_location);
    if (tok_type_declaration != NULL) {
      v->type_declaration = tok_type_declaration->str_val;
      v->type_help = tok_type_declaration->type() == tok_Exception ? tp_Exception : tp_Class;
    }

    if (tp != tp_Unknown) {
      v->type_help = tp;
    }
    res = v;
  }

  return res;
}

VertexPtr GenTree::get_foreach_param () {
  AutoLocation location (this);
  VertexPtr xs = get_expression();
  CE (!kphp_error (xs.not_null(), ""));

  CE (expect (tok_as, "'as'"));
  skip_phpdoc_tokens();

  VertexPtr x, key;
  x = get_var_name_ref();
  CE (!kphp_error (x.not_null(), ""));
  if ((*cur)->type() == tok_double_arrow) {
    next_cur();
    key = x;
    x = get_var_name_ref();
    CE (!kphp_error (x.not_null(), ""));
  }

  vector <VertexPtr> next;
  next.push_back (xs);
  next.push_back (x);
  CREATE_VERTEX(empty, op_empty);
  next.push_back(empty); // will be replaced
  if (key.not_null()) {
    next.push_back (key);
  }
  CREATE_VERTEX (res, op_foreach_param, next);
  set_location (res, location);
  return res;
}

VertexPtr GenTree::conv_to (VertexPtr x, PrimitiveType tp, int ref_flag) {
  if (ref_flag) {
    switch (tp) {
      case tp_array:
        return conv_to_lval <tp_array> (x);
      case tp_int:
        return conv_to_lval <tp_int> (x);
      case tp_var:
        return x;
        break;
      default:
        kphp_error (0, "convert_to not array with ref_flag");
        return x;
    }
  }
  switch (tp) {
    case tp_int:
      return conv_to <tp_int> (x);
    case tp_bool:
      return conv_to <tp_bool> (x);
    case tp_string:
      return conv_to <tp_string> (x);
    case tp_float:
      return conv_to <tp_float> (x);
    case tp_array:
      return conv_to <tp_array> (x);
    case tp_UInt:
      return conv_to <tp_UInt> (x);
    case tp_Long:
      return conv_to <tp_Long> (x);
    case tp_ULong:
      return conv_to <tp_ULong> (x);
    case tp_regexp:
      return conv_to <tp_regexp> (x);
    default:
      return x;
  }
}

VertexPtr GenTree::get_actual_value (VertexPtr v) {
  if (v->type() == op_var && v->extra_type == op_ex_var_const && v->get_var_id().not_null()) {
    return v->get_var_id()->init_val;
  }
  if (v->type() == op_define_val) {
    DefinePtr d = v.as <op_define_val>()->get_define_id();
    if (d->type() == DefineData::def_php) {
      return d->val;
    }
  }
  return v;
}

PrimitiveType GenTree::get_ptype() {
  PrimitiveType tp;
  TokenType tok = (*cur)->type();
  switch (tok) {
    case tok_int:
      tp = tp_int;
      break;
    case tok_string:
      tp = tp_string;
      break;
    case tok_float:
      tp = tp_float;
      break;
    case tok_array:
      tp = tp_array;
      break;
    case tok_bool:
      tp = tp_bool;
      break;
    case tok_var:
      tp = tp_var;
      break;
    case tok_Exception:
      tp = tp_Exception;
      break;
    case tok_func_name:
      tp = get_ptype_by_name ((*cur)->str_val);
      break;
    default:
      tp = tp_Error;
      break;
  }
  if (tp != tp_Error) {
    next_cur();
  }
  return tp;
}

PrimitiveType GenTree::get_type_help (void) {
  PrimitiveType res = tp_Unknown;
  if ((*cur)->type() == tok_triple_colon || (*cur)->type() == tok_triple_colon_begin) {
    bool should_end = (*cur)->type() == tok_triple_colon_begin;
    next_cur();
    res = get_ptype();
    kphp_error (res != tp_Error, "Cannot parse type");
    if (should_end) {
      if ((*cur)->type() == tok_triple_colon_end) {
        next_cur();
      } else {
        kphp_error(false, "Unfinished type hint comment");
      }
    }
  }
  return res;
}

VertexPtr GenTree::get_type_rule_func (void) {
  AutoLocation rule_location (this);
  string_ref name = (*cur)->str_val;
  next_cur();
  CE (expect (tok_lt, "<"));
  vector <VertexPtr> next;
  bool ok_next = gen_list<op_err> (&next, &GenTree::get_type_rule_, tok_comma);
  CE (!kphp_error (ok_next, "Failed get_type_rule_func"));
  CE (expect (tok_gt, ">"));

  CREATE_VERTEX (rule, op_type_rule_func, next);
  set_location (rule, rule_location);
  rule->str_val = name;
  return rule;
}
VertexPtr GenTree::get_type_rule_ (void) {
  PrimitiveType tp = get_ptype();
  TokenType tok = (*cur)->type();
  VertexPtr res = VertexPtr(NULL);
  if (tp != tp_Error) {
    AutoLocation arr_location (this);

    VertexPtr first;
    if (tp == tp_array && tok == tok_lt) {
      next_cur();
      first = get_type_rule_();
      if (kphp_error (first.not_null(), "Cannot parse type_rule (1)")) {
        return VertexPtr();
      }
      CE (expect (tok_gt, "'>'"));
    }

    vector <VertexPtr> next;
    if (first.not_null()) {
      next.push_back (first);
    }
    CREATE_VERTEX (arr, op_type_rule, next);
    arr->type_help = tp;
    set_location (arr, arr_location);
    res = arr;
  } else if (tok == tok_func_name) {
    if ((*cur)->str_val == "lca" || (*cur)->str_val == "OrFalse") {
      res = get_type_rule_func ();
    } else if ((*cur)->str_val == "self") {
      CREATE_VERTEX (self, op_self);
      res = self;
    } else if ((*cur)->str_val == "CONST") {
      next_cur();
      res = get_type_rule_();
      if (res.not_null()) {
        res->extra_type = op_ex_rule_const;
      }
    } else {
      kphp_error (
        0,
        dl_pstr ("Can't parse type_rule. Unknown string [%s]", (*cur)->str_val.c_str())
      );
    }
  } else if (tok == tok_xor) {
    next_cur();
    if (kphp_error (test_expect (tok_int_const), "Int expected")) {
      return VertexPtr();
    }
    int cnt = 0;
    for (const char *s = (*cur)->str_val.begin(), *t = (*cur)->str_val.end(); s != t; s++) {
      cnt = cnt * 10 + *s - '0';
    }
    CREATE_VERTEX (v, op_arg_ref);
    set_location (v, AutoLocation(this));
    v->int_val = cnt;
    res = v;
    next_cur();
    while (test_expect(tok_opbrk)) {
      AutoLocation opbrk_location (this);
      next_cur();
      CE (expect (tok_clbrk, "]"));
      CREATE_VERTEX (index, op_index, res);
      set_location (index, opbrk_location);
      res = index;
    }
  }
  return res;
}

VertexPtr GenTree::get_type_rule (void) {
  VertexPtr res, first;

  TokenType tp = (*cur)->type();
  if (tp == tok_triple_colon || tp == tok_triple_eq ||
      tp == tok_triple_lt || tp == tok_triple_gt || 
      tp == tok_triple_colon_begin || tp == tok_triple_eq_begin ||
      tp == tok_triple_lt_begin || tp == tok_triple_gt_begin) {
    AutoLocation rule_location (this);
    bool should_end = tp == tok_triple_colon_begin || tp == tok_triple_eq_begin ||
                      tp == tok_triple_lt_begin || tp == tok_triple_gt_begin;
    next_cur();
    first = get_type_rule_();

    kphp_error_act (
      first.not_null(),
      "Cannot parse type rule",
      return VertexPtr()
    );

    CREATE_META_VERTEX_1 (rule, meta_op_base, OpInfo::tok_to_op[tp], first);

    if (should_end) {
      if ((*cur)->type() == tok_triple_colon_end) {
        next_cur();
      } else {
        kphp_error (false, "Unfinished type hint comment");
      }
    }

    set_location (rule, rule_location);
    res = rule;
  }
  return res;
}

void GenTree::func_force_return (VertexPtr root, VertexPtr val) {
  if (root->type() != op_function) {
    return;
  }
  VertexAdaptor <op_function> func = root;

  VertexPtr cmd = func->cmd();
  assert (cmd->type() == op_seq);

  bool no_result = val.is_null();
  if (no_result) {
    CREATE_VERTEX (return_val, op_null);
    val = return_val;
  }

  CREATE_VERTEX (return_node, op_return, val);
  return_node->void_flag = no_result;
  vector <VertexPtr> next = cmd->get_next();
  next.push_back (return_node);
  CREATE_VERTEX (seq, op_seq, next);
  func->cmd() = seq;
}

VertexPtr GenTree::create_vertex_this (const AutoLocation &location, bool with_type_rule) {
  CREATE_VERTEX (this_var, op_var);
  this_var->str_val = "this";
  this_var->extra_type = op_ex_var_this;
  this_var->const_type = cnst_const_val;
  set_location(this_var, location);

  if (with_type_rule) {
    CREATE_VERTEX(rule_this_var, op_class_type_rule);
    rule_this_var->type_help = tp_Class;

    CREATE_META_VERTEX_1 (type_rule, meta_op_base, op_common_type_rule, rule_this_var);
    this_var->type_rule = type_rule;

    cur_class().this_type_rules.push_back(rule_this_var);
  }

  return this_var;
}

// __construct(args) { body } => __construct(args) { $this ::: tp_Class; body; return $this; }
void GenTree::patch_func_constructor (VertexAdaptor <op_function> func, const ClassInfo &cur_class) {
  const AutoLocation location(this);
  VertexPtr cmd = func->cmd();
  assert (cmd->type() == op_seq);

  CREATE_VERTEX (return_node, op_return, create_vertex_this(location));
  set_location(return_node, location);

  std::vector <VertexPtr> next = cmd->get_next();
  next.insert(next.begin(), create_vertex_this(location, true));
  next.push_back(return_node);

  for (std::vector <VertexPtr>::const_iterator i = cur_class.members.begin(); i != cur_class.members.end(); ++i) {
    VertexPtr var = *i;
    if (var->type() == op_class_var && var.as <op_class_var>()->def_val.not_null()) {
      CREATE_VERTEX(inst_prop, op_instance_prop, create_vertex_this(location));
      inst_prop->str_val = var->get_string();

      CREATE_VERTEX(set_c1_node, op_set, inst_prop, var.as <op_class_var>()->def_val);
      next.insert(next.begin() + 1, set_c1_node);
    }
  }

  CREATE_VERTEX (seq, op_seq, next);
  func->cmd() = seq;
}

// function fname(args) => function fname($this ::: class_instance, args)
void GenTree::patch_func_add_this (vector <VertexPtr> &params_next, const AutoLocation &func_location) {
  CREATE_VERTEX (param, op_func_param, create_vertex_this(func_location, true));
  params_next.push_back(param);
}

FunctionPtr GenTree::create_default_constructor (const ClassInfo &cur_class) {
  CREATE_VERTEX (func_name, op_func_name);
  func_name->str_val = replace_characters(class_context, '\\', '$') + "$$__construct";
  CREATE_VERTEX (func_params, op_func_param_list);
  CREATE_VERTEX (func_root, op_seq);
  CREATE_VERTEX (func, op_function, func_name, func_params, func_root);
  func->extra_type = op_ex_func_member;
  func->inline_flag = true;

  patch_func_constructor(func, cur_class);

  return register_function(FunctionInfo(
      func, namespace_name, cur_class.name, class_context, this->namespace_uses, class_extends, false, access_public
  ));
}

template <Operation Op>
VertexPtr GenTree::get_multi_call (GetFunc f) {
  TokenType type = (*cur)->type();
  AutoLocation seq_location (this);
  next_cur();

  vector <VertexPtr> next;
  bool ok_next = gen_list<op_err> (&next, f, tok_comma);
  CE (!kphp_error (ok_next, "Failed get_multi_call"));

  for (int i = 0, ni = (int)next.size(); i < ni; i++) {
    if (type == tok_echo || type == tok_dbg_echo) {
      next[i] = conv_to <tp_string> (next[i]);
    }
    CREATE_VERTEX (v, Op, next[i]);
    ::set_location (v, next[i]->get_location());
    next[i] = v;
  }
  CREATE_VERTEX (seq, op_seq, next);
  set_location (seq, seq_location);
  return seq;
}


VertexPtr GenTree::get_return() {
  AutoLocation ret_location (this);
  next_cur();
  skip_phpdoc_tokens();
  VertexPtr return_val = get_expression();
  bool no_result = false;
  if (return_val.is_null()) {
    CREATE_VERTEX (tmp, op_null);
    set_location (tmp, AutoLocation(this));
    return_val = tmp;
    no_result = true;
  }
  CREATE_VERTEX (ret, op_return, return_val);
  set_location (ret, ret_location);
  CE (expect (tok_semicolon, "';'"));
  ret->void_flag = no_result;
  return ret;
}

VertexPtr GenTree::get_exit() {
  AutoLocation exit_location (this);
  next_cur();
  bool is_opened = open_parent();
  VertexPtr exit_val;
  if (is_opened) {
    exit_val = get_expression();
    close_parent (is_opened);
  }
  if (exit_val.is_null()) {
    CREATE_VERTEX (tmp, op_int_const);
    tmp->str_val = "0";
    exit_val = tmp;
  }
  CREATE_VERTEX (v, op_exit, exit_val);
  set_location (v, exit_location);
  return v;
}

template <Operation Op>
VertexPtr GenTree::get_break_continue() {
  AutoLocation res_location (this);
  next_cur();
  VertexPtr first_node = get_expression();
  CE (expect (tok_semicolon, "';'"));

  if (first_node.is_null()) {
    CREATE_VERTEX (one, op_int_const);
    one->str_val = "1";
    first_node = one;
  }

  CREATE_VERTEX (res, Op, first_node);
  set_location (res, res_location);
  return res;
}

VertexPtr GenTree::get_foreach() {
  AutoLocation foreach_location (this);
  next_cur();

  CE (expect (tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr first_node = get_foreach_param();
  CE (!kphp_error (first_node.not_null(), "Failed to parse 'foreach' params"));

  CE (expect (tok_clpar, "')'"));

  VertexPtr second_node = get_statement();
  CE (!kphp_error (second_node.not_null(), "Failed to parse 'foreach' body"));

  CREATE_VERTEX(temp_node, op_empty);

  CREATE_VERTEX (foreach, op_foreach, first_node, embrace (second_node), temp_node);
  set_location (foreach, foreach_location);
  return foreach;
}

VertexPtr GenTree::get_while() {
  AutoLocation while_location (this);
  next_cur();
  CE (expect (tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr first_node = get_expression();
  CE (!kphp_error (first_node.not_null(), "Failed to parse 'while' condition"));
  first_node = conv_to <tp_bool> (first_node);
  CE (expect (tok_clpar, "')'"));

  VertexPtr second_node = get_statement();
  CE (!kphp_error (second_node.not_null(), "Failed to parse 'while' body"));

  CREATE_VERTEX (while_vertex, op_while, first_node, embrace (second_node));
  set_location (while_vertex, while_location);
  return while_vertex;
}

VertexPtr GenTree::get_if() {
  AutoLocation if_location (this);
  VertexPtr if_vertex;
  next_cur();
  CE (expect (tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr first_node = get_expression();
  CE (!kphp_error (first_node.not_null(), "Failed to parse 'if' condition"));
  first_node = conv_to <tp_bool> (first_node);
  CE (expect (tok_clpar, "')'"));

  VertexPtr second_node = get_statement();
  CE (!kphp_error (second_node.not_null(), "Failed to parse 'if' body"));
  second_node = embrace (second_node);

  VertexPtr third_node = VertexPtr();
  if ((*cur)->type() == tok_else) {
    next_cur();
    third_node = get_statement();
    CE (!kphp_error (third_node.not_null(), "Failed to parse 'else' statement"));
  }

  if (third_node.not_null()) {
    third_node = embrace (third_node);
    CREATE_VERTEX (v, op_if, first_node, second_node, third_node);
    if_vertex = v;
  } else {
    CREATE_VERTEX (v, op_if, first_node, second_node);
    if_vertex = v;
  }
  set_location (if_vertex, if_location);
  return if_vertex;
}

VertexPtr GenTree::get_for() {
  AutoLocation for_location (this);
  next_cur();
  CE (expect (tok_oppar, "'('"));
  skip_phpdoc_tokens();

  AutoLocation pre_cond_location (this);
  vector <VertexPtr> first_next;
  bool ok_first_next = gen_list <op_err> (&first_next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error (ok_first_next, "Failed to parse 'for' precondition"));
  CREATE_VERTEX (pre_cond, op_seq, first_next);
  set_location (pre_cond, pre_cond_location);

  CE (expect (tok_semicolon, "';'"));

  AutoLocation cond_location (this);
  vector <VertexPtr> second_next;
  bool ok_second_next = gen_list <op_err> (&second_next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error (ok_second_next, "Failed to parse 'for' action"));
  if (second_next.empty()) {
    CREATE_VERTEX (v_true, op_true);
    second_next.push_back (v_true);
  } else {
    second_next.back() = conv_to <tp_bool> (second_next.back());
  }
  CREATE_VERTEX (cond, op_seq_comma, second_next);
  set_location (cond, cond_location);

  CE (expect (tok_semicolon, "';'"));

  AutoLocation post_cond_location (this);
  vector <VertexPtr> third_next;
  bool ok_third_next = gen_list <op_err> (&third_next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error (ok_third_next, "Failed to parse 'for' postcondition"));
  CREATE_VERTEX (post_cond, op_seq, third_next);
  set_location (post_cond, post_cond_location);

  CE (expect (tok_clpar, "')'"));

  VertexPtr cmd = get_statement();
  CE (!kphp_error (cmd.not_null(), "Failed to parse 'for' statement"));

  cmd = embrace (cmd);
  CREATE_VERTEX (for_vertex, op_for, pre_cond, cond, post_cond, cmd);
  set_location (for_vertex, for_location);
  return for_vertex;
}

VertexPtr GenTree::get_do() {
  AutoLocation do_location (this);
  next_cur();
  VertexPtr first_node = get_statement();
  CE (!kphp_error (first_node.not_null(), "Failed to parser 'do' condition"));

  CE (expect (tok_while, "'while'"));

  CE (expect (tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr second_node = get_expression();
  CE (!kphp_error (second_node.not_null(), "Faild to parse 'do' statement"));
  second_node = conv_to <tp_bool> (second_node);
  CE (expect (tok_clpar, "')'"));
  CE (expect (tok_semicolon, "';'"));

  CREATE_VERTEX (do_vertex, op_do, second_node, first_node);
  set_location (do_vertex, do_location);
  return do_vertex;
}

VertexPtr GenTree::get_switch() {
  AutoLocation switch_location (this);
  vector <VertexPtr> switch_next;

  next_cur();
  CE (expect (tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr switch_val = get_expression();
  CE (!kphp_error (switch_val.not_null(), "Failed to parse 'switch' expression"));
  switch_next.push_back (switch_val);
  CE (expect (tok_clpar, "')'"));

  CE (expect (tok_opbrc, "'{'"));

  // they will be replaced by vars later. 
  // It can't be done now, gen_name is not working here. 
  CREATE_VERTEX(temp_ver1, op_empty); switch_next.push_back(temp_ver1);
  CREATE_VERTEX(temp_ver2, op_empty); switch_next.push_back(temp_ver2);
  CREATE_VERTEX(temp_ver3, op_empty); switch_next.push_back(temp_ver3);
  CREATE_VERTEX(temp_ver4, op_empty); switch_next.push_back(temp_ver4);

  while ((*cur)->type() != tok_clbrc) {
    skip_phpdoc_tokens();
    TokenType cur_type = (*cur)->type();
    VertexPtr case_val;

    AutoLocation case_location (this);
    if (cur_type == tok_case) {
      next_cur();
      case_val = get_expression();
      CE (!kphp_error (case_val.not_null(), "Failed to parse 'case' value"));

      CE (expect2 (tok_colon, tok_semicolon, "':'"));
    }
    if (cur_type == tok_default) {
      next_cur();
      CE (expect2 (tok_colon, tok_semicolon, "':'"));
    }

    AutoLocation seq_location (this);
    vector <VertexPtr> seq_next;
    while (cur != end) {
      if ((*cur)->type() == tok_clbrc) {
        break;
      }
      if ((*cur)->type() == tok_case) {
        break;
      }
      if ((*cur)->type() == tok_default) {
        break;
      }
      VertexPtr cmd = get_statement();
      if (cmd.not_null()) {
        seq_next.push_back (cmd);
      }
    }

    CREATE_VERTEX (seq, op_seq, seq_next);
    set_location (seq, seq_location);
    if (cur_type == tok_case) {
      CREATE_VERTEX (case_block, op_case, case_val, seq);
      set_location (case_block, case_location);
      switch_next.push_back (case_block);
    } else if (cur_type == tok_default) {
      CREATE_VERTEX (case_block, op_default, seq);
      set_location (case_block, case_location);
      switch_next.push_back (case_block);
    }
  }

  CREATE_VERTEX (switch_vertex, op_switch, switch_next);
  set_location (switch_vertex, switch_location);

  CE (expect (tok_clbrc, "'}'"));
  return switch_vertex;
}

VertexPtr GenTree::get_function (bool anonimous_flag, Token *phpdoc_token, AccessType access_type) {
  AutoLocation func_location (this);

  TokenType type = (*cur)->type();
  kphp_assert (test_expect (tok_function) || test_expect (tok_ex_function));
  next_cur();

  string name_str;
  AutoLocation name_location (this);
  if (anonimous_flag) {
    string tmp_name = gen_anonymous_function_name ();
    name_str = tmp_name;
  } else {
    CE (expect (tok_func_name, "'tok_func_name'"));
    cur--;
    name_str = (*cur)->str_val;
    next_cur();
  }

  bool is_instance_method = access_type == access_private || access_type == access_protected || access_type == access_public;
  bool is_constructor = is_instance_method && name_str == "__construct";

  if (in_class()) {
    string full_class_name = replace_backslashes(namespace_name) + "$" + cur_class().name;
    string full_context_name = replace_backslashes(class_context);
    name_str = full_class_name + "$$" + name_str;
    if (full_class_name != full_context_name) {
      name_str += "$$" + full_context_name;
    }
  }
  CREATE_VERTEX (name, op_func_name);
  set_location (name, name_location);
  name->str_val = name_str;

  vertex_inner<meta_op_base> flags_inner;
  VertexPtr flags(&flags_inner);
  if (test_expect (tok_auto)) {
    next_cur();
    flags->auto_flag = true;
  }

  CE (expect (tok_oppar, "'('"));

  AutoLocation params_location (this);
  vector <VertexPtr> params_next;

  if (is_instance_method && !is_constructor) {
    patch_func_add_this(params_next, func_location);
  }

  if (test_expect (tok_varg)) {
    flags->varg_flag = true;
    next_cur();
  } else {
    bool ok_params_next = gen_list<op_err> (&params_next, &GenTree::get_func_param, tok_comma);
    CE (!kphp_error (ok_params_next, "Failed to parse function params"));
  }
  CREATE_VERTEX (params, op_func_param_list, params_next);
  set_location (params, params_location);

  CE (expect (tok_clpar, "')'"));

  bool flag = true;
  while (flag) {
    flag = false;
    if (test_expect (tok_throws)) {
      flags->throws_flag = true;
      CE (expect (tok_throws, "'throws'"));
      flag = true;
    }
    if (test_expect (tok_resumable)) {
      flags->resumable_flag = true;
      CE (expect (tok_resumable, "'resumable'"));
      flag = true;
    }
  }

  VertexPtr type_rule = get_type_rule();

  VertexPtr cmd;
  if ((*cur)->type() == tok_opbrc) {
    enter_function();
    is_top_of_the_function_ = in_func_cnt_ > 1;
    cmd = get_statement();
    exit_function();
    kphp_error (type != tok_ex_function, "Extern function header should not have a body");
    CE (!kphp_error (cmd.not_null(), "Failed to parse function body"));
  } else {
    CE (expect (tok_semicolon, "';'"));
  }


  if (cmd.not_null()) {
    cmd = embrace (cmd);
  }

  bool kphp_required_flag = false;

  // тут раньше был парсинг '@kphp-' тегов в phpdoc, но ему не место в gentree, он переехал в PrepareFunctionF
  // но! костыль: @kphp-required нам всё равно нужно именно тут, чтобы функция пошла дальше по пайплайну
  if (phpdoc_token != NULL && phpdoc_token->type() == tok_phpdoc_kphp) {
    kphp_required_flag = phpdoc_token->str_val.str().find("@kphp-required") != string::npos;
  }

  set_location(flags, func_location);
  flags->type_rule = type_rule;

  FunctionInfo info;
  {
    VertexPtr res = create_function_vertex_with_flags(name, params, flags, type, cmd, is_constructor);
    set_extra_type(res, access_type);
    info = FunctionInfo(res, "", "", "", this->namespace_uses, "", kphp_required_flag, access_type);
    fill_info_about_class(info);

    register_function(info);

    if (info.root->type() == op_function) {
      info.root->get_func_id()->access_type = access_type;
      info.root->get_func_id()->phpdoc_token = phpdoc_token;
    }

    if (is_constructor) {
      cur_class().new_function = info.root->get_func_id();
    }
  }

  if (in_class() && context_class_ptr.not_null()) {
    add_parent_function_to_descendants_with_context(info, access_type, params_next);
  }

  if (stage::has_error()) {
    CE(false);
  }

  if (anonimous_flag) {
    CREATE_VERTEX (func_ptr, op_func_name);
    set_location (func_ptr, name_location);
    func_ptr->str_val = name->str_val;
    return func_ptr;
  }
  return VertexPtr();
}

bool GenTree::check_seq_end() {
  if (!test_expect (tok_clbrc)) {
    kphp_error (0, "Failed to parse sequence");
    while (cur != end && !test_expect (tok_clbrc)) {
      next_cur();
    }
  }
  return expect (tok_clbrc, "'}'");
}

bool GenTree::check_statement_end() {
  //if (test_expect (tok_clbrc)) {
    //return true;
  //}
  if (!test_expect (tok_semicolon)) {
    kphp_error (0, "Failed to parse statement. Expected `;`");
    while (cur != end && !test_expect (tok_clbrc) && !test_expect (tok_semicolon)) {
      next_cur();
    }
  }
  return expect (tok_semicolon, "';'");
}

static inline bool is_class_name_allowed(const string &name) {
  static string a[] = {"Exception", "RpcMemcache", "Memcache", "rpc_connection", "Long", "ULong", "UInt", "true_mc", "test_mc", "rich_mc", "db_decl"};
  static set<string> disallowed_names(a, a + sizeof(a) / sizeof(a[0]));

  return disallowed_names.find(name) == disallowed_names.end();
}

VertexPtr GenTree::get_class (Token *phpdoc_token) {
  AutoLocation class_location (this);
  CE (expect (tok_class, "'class'"));
  CE (!kphp_error (test_expect (tok_func_name), "Class name expected"));

  string name_str = (*cur)->str_val;
  if (in_namespace()) {
    string expected_name = stage::get_file()->short_file_name;
    kphp_error (name_str == expected_name,
                dl_pstr("Expected class name %s, found %s", expected_name.c_str(), name_str.c_str()));
    if (!is_class_name_allowed(name_str)) {
      kphp_error (false, dl_pstr("Sorry, kPHP doesn't support class name %s", name_str.c_str()));
    }
    if (class_context.empty()) {
      class_context = namespace_name + "\\" + name_str;
    }
  }
  CREATE_VERTEX (name, op_func_name);
  set_location (name, AutoLocation (this));
  name->str_val = name_str;

  next_cur();

  VertexPtr parent_name;

  if (test_expect(tok_extends)) {
    next_cur();
    CE (!kphp_error (test_expect(tok_func_name), "Class name expected after 'extends'"));
    CREATE_VERTEX (tmp, op_func_name);
    set_location (tmp, AutoLocation (this));
    tmp->str_val = (*cur)->str_val;
    class_extends = (*cur)->str_val;
    parent_name = tmp;
    next_cur();
  }

  enter_class(name_str, phpdoc_token);
  VertexPtr class_body = get_statement();
  CE (!kphp_error (class_body.not_null(), "Failed to parse class body"));

  CREATE_VERTEX (class_vertex, op_class, name, parent_name);
  set_location (class_vertex, class_location);

  exit_and_register_class (class_vertex);
  return VertexPtr();
}

VertexPtr GenTree::get_use() {
  kphp_assert(test_expect(tok_use));
  next_cur();
  while (true) {
    if (!test_expect(tok_func_name)) {
      expect(tok_func_name, "<namespace path>");
    }
    string name = (*cur)->str_val;
    kphp_assert(!name.empty());
    if (name[0] == '\\') {
      name = name.substr(1);
    }
    string alias = name.substr(name.rfind('\\') + 1);
    kphp_error(!alias.empty(), "KPHP doesn't support use of global namespace");
    next_cur();
    if (test_expect(tok_as)) {
      next_cur();
      if (!test_expect(tok_func_name)) {
        expect(tok_func_name, "<use alias>");
      }
      alias = (*cur)->str_val;
      next_cur();
    }
    map<string, string> &uses = this->namespace_uses;
    if (uses.find(alias) == uses.end()) {
      uses[alias] = name;
    }
    if (!test_expect(tok_comma)) {
      break;
    }
    next_cur();
  }
  expect2(tok_semicolon, tok_comma, "';' or ','");
  return VertexPtr();
}

VertexPtr GenTree::get_namespace_class() {
  kphp_assert (test_expect (tok_namespace));
  next_cur();
  kphp_error (test_expect (tok_func_name), "Namespace name expected");
  SrcFilePtr current_file = stage::get_file();
  string namespace_name = (*cur)->str_val;
  string expected_namespace_name = replace_characters(current_file->unified_dir_name, '/', '\\');

  kphp_error (namespace_name == expected_namespace_name,
                  dl_pstr("Wrong namespace name, expected %s", expected_namespace_name.c_str()));
  this->namespace_name = namespace_name;
  next_cur();
  expect (tok_semicolon, "';'");
  if (stage::has_error()) {
    while (cur != end) ++cur;
    return VertexPtr();
  }
  while (test_expect(tok_use)) {
    get_use();
  }

  Token *phpdoc_token = NULL;
  if ((*cur)->type() == tok_phpdoc || (*cur)->type() == tok_phpdoc_kphp) {
    phpdoc_token = *cur;
    next_cur();
  }
  VertexPtr cv = get_class(phpdoc_token);
  CE (check_statement_end());
  this->namespace_name = "";
  this->namespace_uses.clear();
  return cv;
}

VertexPtr GenTree::get_statement (Token *phpdoc_token) {
  VertexPtr res, first_node, second_node, third_node, forth_node, tmp_node;
  TokenType type = (*cur)->type();

  VertexPtr type_rule;
  is_top_of_the_function_ &= vk::any_of_equal(type, tok_global, tok_opbrc);

  switch (type) {
    case tok_class:
      res = get_class(phpdoc_token);
      return VertexPtr();
    case tok_opbrc:
      next_cur();
      res = get_seq();
      kphp_error (res.not_null(), "Failed to parse sequence");
      CE (check_seq_end());
      return res;
    case tok_return:
      res = get_return();
      return res;
    case tok_continue:
      res = get_break_continue <op_continue>();
      return res;
    case tok_break:
      res = get_break_continue <op_break>();
      return res;
    case tok_unset:
      res = get_func_call <op_unset, op_err>();
      CE (check_statement_end());
      return res;
    case tok_var_dump:
      res = get_func_call <op_var_dump, op_err>();
      CE (check_statement_end());
      return res;
    case tok_define:
      res = get_func_call <op_define, op_err>();
      CE (check_statement_end());
      return res;
    case tok_define_raw:
      res = get_func_call <op_define_raw, op_err>();
      CE (check_statement_end());
      return res;
    case tok_global:
      if (G->env().get_warnings_level() >= 1 && in_func_cnt_ > 1 && !is_top_of_the_function_) {
        kphp_warning("`global` keyword is allowed only at the top of the function");
      }
      res = get_multi_call <op_global>(&GenTree::get_var_name);
      CE (check_statement_end());
      return res;
    case tok_static:

      if (cur != end) {
        Token *next_token = *(cur + 1);
        std::string error_msg = "Expected `function` or variable name after keyword `static`, but got: " + next_token->str_val.str();
        if (kphp_error(next_token->type() == tok_function || next_token->type() == tok_var_name, error_msg.c_str())) {
          next_cur();
          CE(false);
        }
      }

      res = get_multi_call <op_static>(&GenTree::get_expression);
      CE (check_statement_end());
      return res;
    case tok_echo:
      res = get_multi_call <op_echo>(&GenTree::get_expression);
      CE (check_statement_end());
      return res;
    case tok_dbg_echo:
      res = get_multi_call <op_dbg_echo>(&GenTree::get_expression);
      CE (check_statement_end());
      return res;
    case tok_throw: {
      AutoLocation throw_location (this);
      next_cur();
      first_node = get_expression();
      CE (!kphp_error (first_node.not_null(), "Empty expression in throw"));
      CREATE_VERTEX (throw_vertex, op_throw, first_node);
      set_location (throw_vertex, throw_location);
      CE (check_statement_end());
      return throw_vertex;
    }

    case tok_while:
      return get_while();
    case tok_if:
      return get_if();
    case tok_for:
      return get_for();
    case tok_do:
      return get_do();
    case tok_foreach:
      return get_foreach();
    case tok_switch:
      return get_switch();
    case tok_protected:
    case tok_public:
    case tok_private: {
      CE (!kphp_error(in_class(), "Access modifier used outside of class"));
      next_cur();
      TokenType cur_tok = cur == end ? tok_semicolon : (*cur)->type();
      TokenType next_tok = cur == end || cur + 1 == end ? tok_semicolon : (*(cur + 1))->type();
      AccessType access_type =
          cur_tok == tok_static
          ? (type == tok_public ? access_static_public :
             type == tok_private ? access_static_private : access_static_protected)
          : (type == tok_public ? access_public :
             type == tok_private ? access_private : access_protected);
      OperationExtra extra_type =
          type == tok_private ? op_ex_static_private :
          type == tok_public ? op_ex_static_public : op_ex_static_protected;

      // не статическая функция (public function ...)
      if (cur_tok == tok_function) {
        return get_function(false, phpdoc_token, access_type);
      }
      // статическая функция (public static function ...)
      if (next_tok == tok_function) {
        expect (tok_static, "'static'");
        return get_function(false, phpdoc_token, access_type);
      }
      // не статическое свойство (public $var1 [=default] [,$var2...])
      if (cur_tok == tok_var_name) {
        return get_vars_list(phpdoc_token, extra_type);
      }
      // статическое свойство (public static $staticVar)
      VertexPtr v = get_statement(phpdoc_token);
      CE(v.not_null());
      for(auto e : *v) {
        kphp_assert(e->type() == op_static);
        e->extra_type = extra_type;
        VertexAdaptor <op_static> seq = e;
        for (auto node : seq->args()) {
          VertexAdaptor <op_var> var;
          if (node->type() == op_var) {
            var = node;
          } else if (node->type() == op_set) {
            VertexAdaptor <op_set> set_expr = node;
            var = set_expr->lhs();
            kphp_error_act (
              var->type() == op_var,
              "unexpected expression in 'static'",
              continue
            );
          } else {
            kphp_error_act (0, "unexpected expression in 'static'", continue);
          }
          kphp_error(cur_class().static_fields.insert(var->str_val).second,
                     dl_pstr("static field %s was redeclared", var->str_val.c_str()));
        }
      }
      cur_class().static_members.push_back(v);
      CREATE_VERTEX (empty, op_empty);
      return empty;
    }
    case tok_phpdoc_kphp:
    case tok_phpdoc: {
      Token *token = *cur;
      next_cur();
      return get_statement(token);
    }
    case tok_ex_function:
    case tok_function:
      return get_function(false, phpdoc_token);

    case tok_try: {
      AutoLocation try_location (this);
      next_cur();
      first_node = get_statement();
      CE (!kphp_error (first_node.not_null(), "Cannot parse try block"));
      CE (expect (tok_catch, "'catch'"));
      CE (expect (tok_oppar, "'('"));
      CE (expect (tok_Exception, "'Exception'"));
      second_node = get_expression();
      CE (!kphp_error (second_node.not_null(), "Cannot parse catch ( ??? )"));
      CE (!kphp_error (second_node->type() == op_var, "Expected variable name in 'catch'"));
      second_node->type_help = tp_Exception;

      CE (expect (tok_clpar, "')'"));
      third_node = get_statement();
      CE (!kphp_error (third_node.not_null(), "Cannot parse catch block"));
      CREATE_VERTEX (try_vertex, op_try, embrace (first_node), second_node, embrace (third_node));
      set_location (try_vertex, try_location);
      return try_vertex;
    }
    case tok_break_file: {
      CREATE_VERTEX (v, op_break_file);
      set_location (v, AutoLocation(this));
      next_cur();
      return v;
    }
    case tok_inline_html: {
      CREATE_VERTEX (html_code, op_string);
      set_location (html_code, AutoLocation(this));
      html_code->str_val = (*cur)->str_val;

      CREATE_VERTEX (echo_cmd, op_echo, html_code);
      set_location (echo_cmd, AutoLocation(this));
      next_cur();
      return echo_cmd;
    }
    case tok_at: {
      AutoLocation noerr_location (this);
      next_cur();
      first_node = get_statement();
      CE (first_node.not_null());
      CREATE_VERTEX (noerr, op_noerr, first_node);
      set_location (noerr, noerr_location);
      return noerr;
    }
    case tok_clbrc: {
      return res;
    }
    case tok_const: {
      AutoLocation const_location (this);
      next_cur();

      bool has_access_modifier = std::distance(tokens->begin(), cur) > 1 && vk::any_of_equal((*std::prev(cur, 2))->type(), tok_public, tok_private, tok_protected);
      bool const_in_global_scope = in_func_cnt_ == 1 && !in_class() && !in_namespace();
      bool const_in_class = in_func_cnt_ == 0 && in_class() && in_namespace();

      CE (!kphp_error(const_in_global_scope || const_in_class, "const expressions supported only inside classes and namespaces or in global scope"));
      CE (!kphp_error(test_expect(tok_func_name), "expected constant name"));
      CE (!kphp_error(!has_access_modifier, "unexpected const after private/protected/public keyword"));

      CREATE_VERTEX (name, op_func_name);
      string const_name = (*cur)->str_val;

      if (const_in_class) {
        name->str_val = "c#" + replace_backslashes(namespace_name) + "$" + cur_class().name + "$$" + const_name;
      } else {
        name->str_val = const_name;
      }

      next_cur();
      CE (expect(tok_eq1, "'='"));
      VertexPtr v = get_expression();
      CREATE_VERTEX(def, op_define, name, v);
      set_location(def, const_location);
      CE (check_statement_end());

      if (const_in_class) {
        kphp_error(cur_class().constants.find(const_name) == cur_class().constants.end(), dl_pstr("Redeclaration of const %s", const_name.c_str()));
        cur_class().constants[const_name] = def;
        CREATE_VERTEX (empty, op_empty);
        return empty;
      }

      return def;
    }
    case tok_use: {
      AutoLocation const_location (this);
      CE (!kphp_error(!in_class() && in_func_cnt_ == 1, "'use' can be declared only in global scope"));
      get_use();
      CREATE_VERTEX (empty, op_empty);
      return empty;
    }
    case tok_var: {
      next_cur();
      get_vars_list(phpdoc_token, op_ex_static_public);
      CE (check_statement_end());
      CREATE_VERTEX(empty, op_empty);
      return empty;
    }
    default:
      res = get_expression();
      if (res.is_null()) {
        if ((*cur)->type() == tok_semicolon) {
          CREATE_VERTEX (empty, op_empty);
          set_location (empty, AutoLocation(this));
          res = empty;
        } else if (phpdoc_token) {
          return res;
        } else {
          CE (check_statement_end());
          return res;
        }
      } else {
        type_rule = get_type_rule();
        res->type_rule = type_rule;
        if (res->type() == op_set) {
          res.as <op_set>()->phpdoc_token = phpdoc_token;
        }
      }
      CE (check_statement_end());
      //CE (expect (tok_semicolon, "';'"));
      return res;
  }
  kphp_fail();
}

VertexPtr GenTree::get_vars_list (Token *phpdoc_token, OperationExtra extra_type) {
  kphp_error(in_class(), "var declaration is outside of class");

  const std::string &var_name = (*cur)->str_val;
  CE (expect(tok_var_name, "expected variable name"));
  CREATE_VERTEX (var, op_class_var);
  var->extra_type = extra_type;       // чтобы в create_class() определить, это public/private/protected
  var->str_val = var_name;
  var->phpdoc_token = phpdoc_token;

  if (test_expect(tok_eq1)) {
    next_cur();
    var->def_val = get_expression();
  }

  cur_class().vars.push_back(var);
  cur_class().members.push_back(var);
  //printf("var %s in class %s\n", var_name.c_str(), cur_class().name.c_str());

  if (test_expect(tok_comma)) {
    next_cur();
    get_vars_list(phpdoc_token, extra_type);
  }

  return VertexPtr();
}

VertexPtr GenTree::get_seq() {
  vector <VertexPtr> seq_next;
  AutoLocation seq_location (this);

  while (cur != end && !test_expect (tok_clbrc)) {
    VertexPtr cur_node = get_statement();
    if (cur_node.is_null()) {
      continue;
    }
    seq_next.push_back (cur_node);
  }
  CREATE_VERTEX (seq, op_seq, seq_next);
  set_location (seq, seq_location);

  return seq;
}

bool GenTree::is_superglobal (const string &s) {
  static set <string> names;
  static bool is_inited = false;
  if (!is_inited) {
    is_inited = true;
    names.insert ("_SERVER");
    names.insert ("_GET");
    names.insert ("_POST");
    names.insert ("_FILES");
    names.insert ("_COOKIE");
    names.insert ("_REQUEST");
    names.insert ("_ENV");
  }
  return names.count (s);
}

bool GenTree::has_return (VertexPtr v) {
  return v->type() == op_return || std::any_of(v->begin(), v->end(), has_return);
}

VertexPtr GenTree::post_process (VertexPtr root) {
  if (root->type() == op_func_call && root->size() == 1) {
    VertexAdaptor <op_func_call> call = root;
    string str = call->get_string();

    Operation op = op_err;
    if (str == "strval") {
      op = op_conv_string;
    } else if (str == "intval") {
      op = op_conv_int;
    } else if (str == "boolval") {
      op = op_conv_bool;
    } else if (str == "floatval") {
      op = op_conv_float;
    } else if (str == "arrayval") {
      op = op_conv_array;
    } else if (str == "uintval") {
      op = op_conv_uint;
    } else if (str == "longval") {
      op = op_conv_long;
    } else if (str == "ulongval") {
      op = op_conv_ulong;
    } else if (str == "fork") {
      op = op_fork;
    }
    if (op != op_err) {
      VertexPtr arg = call->args()[0];
      if (op == op_fork) {
        arg->fork_flag = true;
      }
      CREATE_META_VERTEX_1 (new_root, meta_op_base, op, arg);
      ::set_location (new_root, root->get_location());
      return post_process (new_root);
    }
  }

  if (root->type() == op_minus || root->type() == op_plus) {
    VertexAdaptor <meta_op_unary_op> minus = root;
    VertexPtr maybe_num = minus->expr();
    string prefix = root->type() == op_minus ? "-" : "";
    if (maybe_num->type() == op_int_const || maybe_num->type() == op_float_const) {
      VertexAdaptor <meta_op_num> num = maybe_num;
      num->str_val = prefix + num->str_val;
      minus->expr() = VertexPtr();
      return post_process (num);
    }
  }

  if (root->type() == op_set) {
    VertexAdaptor <op_set> set_op = root;
    if (set_op->lhs()->type() == op_list_ce) {
      vector <VertexPtr> next;
      next = set_op->lhs()->get_next();
      next.push_back (set_op->rhs());
      CREATE_VERTEX (list, op_list, next);
      ::set_location (list, root->get_location());
      list->phpdoc_token = root.as <op_set>()->phpdoc_token;
      return post_process (list);
    }
  }

  if (root->type() == op_define || root->type() == op_define_raw) {
    VertexAdaptor <meta_op_define> define = root;
    VertexPtr name = define->name();
    if (name->type() == op_func_name) {
      CREATE_VERTEX (new_name, op_string);
      new_name->str_val = name.as <op_func_name>()->str_val;
      ::set_location (new_name, name->get_location());
      define->name() = new_name;
    }
  }

  if (root->type() == op_function && root->get_string() == "requireOnce") {
    CREATE_VERTEX (empty, op_empty);
    return post_process (empty);
  }

  if (root->type() == op_func_call && root->get_string() == "call_user_func_array") {
    VertexRange args = root.as <op_func_call>()->args();
    kphp_error ((int)args.size() == 2, dl_pstr ("Call_user_func_array expected 2 arguments, got %d", (int)root->size()));
    kphp_error_act (args[0]->type() == op_string, "First argument of call_user_func_array must be a const string", return root);
    CREATE_VERTEX (arg, op_varg, args[1]);
    ::set_location (arg, args[1]->get_location());
    CREATE_VERTEX (new_root, op_func_call, arg);
    ::set_location (new_root, arg->get_location());
    new_root->str_val = args[0].as <op_string>()->str_val;
    return post_process (new_root);
  }

  for (auto &i : *root) {
    i = post_process (i);
  }

  if (root->type() == op_var) {
    if (is_superglobal (root->get_string())) {
      root->extra_type = op_ex_var_superglobal;
    }
  }

  if (root->type() == op_arrow) {
    VertexAdaptor <op_arrow> arrow = root;
    VertexPtr rhs = arrow->rhs();

    if (rhs->type() == op_func_name) {
      CREATE_VERTEX(inst_prop, op_instance_prop, arrow->lhs());
      inst_prop->set_string(rhs->get_string());

      root = inst_prop;
    } else if (rhs->type() == op_func_call) {
      vector <VertexPtr> new_next;
      const vector <VertexPtr> &old_next = rhs.as <op_func_call>()->get_next();

      new_next.push_back(arrow->lhs());
      new_next.insert(new_next.end(), old_next.begin(), old_next.end());

      CREATE_VERTEX (new_root, op_func_call, new_next);
      ::set_location(new_root, root->get_location());
      new_root->extra_type = op_ex_func_member;
      new_root->str_val = rhs->get_string();

      root = new_root;
    } else {
      kphp_error (false, "Operator '->' expects property or function call as its right operand");
    }
  }

  return root;
}

VertexPtr GenTree::run() {
  VertexPtr res;
  if (test_expect (tok_namespace)) {
    res = get_namespace_class();
  } else {
    res = get_statement();
  }
  kphp_assert (res.is_null());
  if (cur != end) {
    fprintf (stderr, "line %d: something wrong\n", line_num);
    kphp_error (0, "Cannot compile (probably problems with brace balance)");
  }

  return res;
}

void GenTree::for_each (VertexPtr root, void (*callback) (VertexPtr )) {
  callback (root);

  for (auto i : *root) {
    for_each (i, callback);
  }
}

VertexPtr GenTree::create_function_vertex_with_flags(VertexPtr name, VertexPtr params, VertexPtr flags, TokenType type, VertexPtr cmd, bool is_constructor) {
  VertexPtr res;
  if (cmd.is_null()) {
    if (type == tok_ex_function) {
      CREATE_VERTEX (func, op_extern_func, name, params);
      res = func;
    } else if (type == tok_function) {
      CREATE_VERTEX (func, op_func_decl, name, params);
      res = func;
    }
  } else {
    CREATE_VERTEX (func, op_function, name, params, cmd);
    res = func;
    if (is_constructor) {
      patch_func_constructor(res, cur_class());
    } else {
      func_force_return(res);
    }
  }

  res->copy_location_and_flags(*flags);

  return res;
}

void GenTree::set_extra_type(VertexPtr vertex, AccessType access_type) const {
  if (!in_namespace()) {
    if (access_type != access_nonmember) {
      vertex->extra_type = op_ex_func_member;
    } else if (in_func_cnt_ == 0) {
      vertex->extra_type = op_ex_func_global;
    }
  }
}

void GenTree::fill_info_about_class(FunctionInfo &info) {
  if (in_class()) {
    info.namespace_name = namespace_name;
    info.class_name = cur_class().name;
    info.class_context = class_context;
    info.extends = class_extends;
  }
}

static inline void fill_real_and_full_name(const FunctionInfo &info, string &full_name, string &real_name) {
  kphp_assert(info.root->type() == op_function || info.root->type() == op_extern_func || info.root->type() == op_func_decl);

  full_name = info.root.as<meta_op_function>()->name()->get_string();
  size_t first_dollars_pos = full_name.find("$$") + 2;
  kphp_assert(first_dollars_pos != string::npos);
  size_t second_dollars_pos = full_name.find("$$", first_dollars_pos);
  if (second_dollars_pos == string::npos) {
    second_dollars_pos = full_name.size();
  }

  real_name = full_name.substr(first_dollars_pos, second_dollars_pos - first_dollars_pos);
}

void GenTree::add_parent_function_to_descendants_with_context(FunctionInfo info, AccessType access_type, const vector<VertexPtr> &params_next) {
  string cur_class_name = class_context;
  ClassPtr cur_class = G->get_class(cur_class_name);
  info.fill_namespace_and_class_name(cur_class_name);

  string real_function_name;
  string full_base_class_name;
  fill_real_and_full_name(info, full_base_class_name, real_function_name);

  while (cur_class.not_null() && !cur_class->extends.empty()) {
    kphp_assert(info.namespace_name + "\\" + info.class_name == cur_class_name);

    string new_function_name = get_name_for_new_function_with_parent_call(info, real_function_name);
    FunctionSetPtr fs_new_function = G->get_function_set(fs_function, new_function_name, false);

    if (fs_new_function.is_null() || fs_new_function->size() == 0) {
      kphp_assert(!cur_class_name.empty() && cur_class_name[0] != '\\');

      info.extends = cur_class->extends;

      VertexPtr func = generate_function_with_parent_call(info, real_function_name, params_next);
      if (stage::has_error()) {
        return;
      }

      info.root = func;
      FunctionPtr registered_function = register_function(info);
      if (registered_function.not_null()) {
        info.root->get_func_id()->access_type = access_type;
      }
    }

    cur_class_name = cur_class->extends;

    if (cur_class_name.empty()) {
      break;
    }

    if (cur_class_name[0] == '\\') {
      cur_class_name.erase(0, 1);
    }

    if (replace_backslashes(cur_class_name) == full_base_class_name) {
      break;
    }

    info.fill_namespace_and_class_name(cur_class_name);
    cur_class = G->get_class(cur_class_name);
  }
}

VertexPtr GenTree::generate_function_with_parent_call(FunctionInfo info, const string &real_name, const vector<VertexPtr> &params_next) {
  CREATE_VERTEX(new_name, op_func_name);
  new_name->set_string(get_name_for_new_function_with_parent_call(info, real_name));
  vector <VertexPtr> new_params_next;
  vector <VertexPtr> new_params_call;
  for (const auto &parameter : params_next) {
    if (parameter->type() == op_func_param) {
      CLONE_VERTEX(new_var, op_var, parameter.as<op_func_param>()->var().as<op_var>());
      new_params_call.push_back(new_var);
      new_params_next.push_back(clone_vertex(parameter));
    } else if (parameter->type() == op_func_param_callback) {
      if (!kphp_error(false, "Callbacks are not supported in class static methods")) {
        return VertexPtr();
      }
    }
  }

  CREATE_VERTEX(new_func_call, op_func_call, new_params_call);

  string parent_function_name = replace_backslashes(namespace_name) + "$" + replace_backslashes(cur_class().name) + "$$" + real_name + "$$" + replace_backslashes(class_context);
  // it's equivalent to new_func_call->set_string("parent::" + real_name);
  new_func_call->set_string(parent_function_name);

  CREATE_VERTEX(new_return, op_return, new_func_call);
  CREATE_VERTEX(new_cmd, op_seq, new_return);
  CREATE_VERTEX(new_params, op_func_param_list, new_params_next);
  CREATE_VERTEX(func, op_function, new_name, new_params, new_cmd);
  func->copy_location_and_flags(*info.root);
  func_force_return(func);
  func->inline_flag = true;

  return func;
}


string GenTree::get_name_for_new_function_with_parent_call(const FunctionInfo &info, const string &real_name) {
  string target_class = replace_backslashes(info.namespace_name + "\\" + info.class_name);
  if (target_class == replace_backslashes(info.class_context)) {
    return target_class + "$$" + real_name;
  } else {
    return target_class + "$$" + real_name + "$$" + replace_backslashes(info.class_context);
  }
}

void gen_tree_init() {
  GenTree::is_superglobal("");
}

void php_gen_tree (vector <Token *> *tokens, const string &context, const string &main_func_name __attribute__((unused)), GenTreeCallbackBase *callback) {
  GenTree gen;
  gen.init (tokens, context, callback);
  gen.run();
}

#undef CE
