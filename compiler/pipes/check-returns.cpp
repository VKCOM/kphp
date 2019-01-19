#include "compiler/pipes/check-returns.h"

#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/io.h"

class CheckReturnsPass : public FunctionPassBase {
private:
  bool have_void;
  bool have_not_void;
  bool warn_fired;
  bool error_fired;
public:
  string get_description() {
    return "Check returns";
  }

  void init();

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused)));

  nullptr_t on_finish();
};

VertexPtr CheckReturnsPass::on_exit_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_return) {
    if (root->void_flag) {
      have_void = true;
    } else {
      have_not_void = true;
    }
    if (have_void && have_not_void && !warn_fired) {
      warn_fired = true;
      FunctionPtr fun = stage::get_function();
      if (fun->type() != FunctionData::func_switch && fun->name != fun->file_id->main_func_name) {
        kphp_typed_warning("return", "Mixing void and not void returns in one function");
      }
    }
    if (have_not_void && !error_fired) {
      for (const FunctionData::InferHint &hint : current_function->infer_hints) {
        if (hint.param_i == -1 && hint.type_rule->void_flag) {
          error_fired = true;
          kphp_error(0, "Expected only void return");
        }
      }
    }
  }

  return root;
}
nullptr_t CheckReturnsPass::on_finish() {
  if (!have_not_void && !stage::get_function()->is_extern()) {
    stage::get_function()->root->void_flag = true;
  }
  return {};
}
void CheckReturnsPass::init() {
  have_void = have_not_void = warn_fired = error_fired = false;
}
void CheckReturnsF::execute(FunctionAndCFG function_and_cfg, DataStream<FunctionAndCFG> &os) {
  CheckReturnsPass pass;
  run_function_pass(function_and_cfg.function, &pass);
  if (stage::has_error()) {
    return;
  }
  os << function_and_cfg;
}
