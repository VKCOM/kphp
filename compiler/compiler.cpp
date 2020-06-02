#include "compiler/compiler.h"

#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <ftw.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

#include "common/algorithms/hashes.h"
#include "common/crc32.h"
#include "common/type_traits/function_traits.h"
#include "common/version-string.h"
#include "drinkless/dl-utils-lite.h"

#include "compiler/compiler-core.h"
#include "compiler/lexer.h"
#include "compiler/make/make.h"
#include "compiler/pipes/analyzer.h"
#include "compiler/pipes/calc-actual-edges.h"
#include "compiler/pipes/calc-bad-vars.h"
#include "compiler/pipes/calc-const-types.h"
#include "compiler/pipes/calc-empty-functions.h"
#include "compiler/pipes/calc-locations.h"
#include "compiler/pipes/calc-real-defines-values.h"
#include "compiler/pipes/calc-rl.h"
#include "compiler/pipes/calc-val-ref.h"
#include "compiler/pipes/cfg.h"
#include "compiler/pipes/check-access-modifiers.h"
#include "compiler/pipes/check-classes.h"
#include "compiler/pipes/check-conversions.h"
#include "compiler/pipes/check-function-calls.h"
#include "compiler/pipes/check-modifications-of-const-vars.h"
#include "compiler/pipes/check-nested-foreach.h"
#include "compiler/pipes/check-requires.h"
#include "compiler/pipes/check-restrictions.h"
#include "compiler/pipes/check-tl-classes.h"
#include "compiler/pipes/check-ub.h"
#include "compiler/pipes/clone-strange-const-params.h"
#include "compiler/pipes/code-gen.h"
#include "compiler/pipes/collect-const-vars.h"
#include "compiler/pipes/collect-main-edges.h"
#include "compiler/pipes/collect-required-and-classes.h"
#include "compiler/pipes/convert-list-assignments.h"
#include "compiler/pipes/convert-local-phpdocs.h"
#include "compiler/pipes/erase-defines-declarations.h"
#include "compiler/pipes/extract-async.h"
#include "compiler/pipes/extract-resumable-calls.h"
#include "compiler/pipes/file-to-tokens.h"
#include "compiler/pipes/filter-only-actually-used.h"
#include "compiler/pipes/final-check.h"
#include "compiler/pipes/fix-returns.h"
#include "compiler/pipes/gen-tree-postprocess.h"
#include "compiler/pipes/generate-virtual-methods.h"
#include "compiler/pipes/inline-simple-functions.h"
#include "compiler/pipes/inline-defines-usages.h"
#include "compiler/pipes/load-files.h"
#include "compiler/pipes/optimization.h"
#include "compiler/pipes/parse.h"
#include "compiler/pipes/prepare-function.h"
#include "compiler/pipes/preprocess-break.h"
#include "compiler/pipes/preprocess-eq3.h"
#include "compiler/pipes/preprocess-function.h"
#include "compiler/pipes/preprocess-vararg.h"
#include "compiler/pipes/register-defines.h"
#include "compiler/pipes/register-variables.h"
#include "compiler/pipes/remove-empty-function-calls.h"
#include "compiler/pipes/resolve-self-static-parent.h"
#include "compiler/pipes/sort-and-inherit-classes.h"
#include "compiler/pipes/split-switch.h"
#include "compiler/pipes/transform-to-smart-instanceof.h"
#include "compiler/pipes/type-inferer.h"
#include "compiler/pipes/write-files.h"
#include "compiler/scheduler/constructor.h"
#include "compiler/scheduler/one-thread-scheduler.h"
#include "compiler/scheduler/pipe_with_progress.h"
#include "compiler/scheduler/scheduler.h"
#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

class lockf_wrapper {
  std::string locked_filename_;
  int fd_ = -1;
  bool locked_ = false;

  void close_and_unlink() {
    if (close(fd_) == -1) {
      std::cerr << "Can't close file '" << locked_filename_ << "': " << strerror(errno) << "\n";
      return;
    }

    if (unlink(locked_filename_.c_str()) == -1) {
      std::cerr << "Can't unlink file '" << locked_filename_ << "': " << strerror(errno) << "\n";
      return;
    }
  }

public:
  bool lock() {
    std::string dest_path = G->env().get_dest_dir();
    if (G->env().get_use_auto_dest()) {
      dest_path += G->get_subdir_name();
    }

    std::stringstream ss;
    ss << "/tmp/" << std::hex << vk::std_hash(dest_path) << "_kphp_temp_lock";
    locked_filename_ = ss.str();

    fd_ = open(locked_filename_.c_str(), O_RDWR | O_CREAT, 0666);
    assert(fd_ != -1);

    locked_ = true;
    if (lockf(fd_, F_TLOCK, 0) == -1) {
      std::cerr << "\nCan't lock file, maybe you have already run kphp2cpp: " << strerror(errno) << "\n";
      close(fd_);
      locked_ = false;
    }

    return locked_;
  }

  ~lockf_wrapper() {
    if (locked_) {
      if (lockf(fd_, F_ULOCK, 0) == -1) {
        std::cerr << "Can't unlock file '" << locked_filename_ << "': " << strerror(errno) << "\n";
      }

      close_and_unlink();
      locked_ = false;
    }
  }
};

template<typename F>
using ExecuteFunctionArguments = vk::function_traits<decltype(&F::execute)>;
template<typename F>
using ExecuteFunctionInput = typename ExecuteFunctionArguments<F>::template Argument<0>;
template<typename F>
using ExecuteFunctionOutput = typename std::remove_reference<typename ExecuteFunctionArguments<F>::template Argument<1>>::type;

template<class PipeFunctionT>
using PipeStream = PipeWithProgress<
  PipeFunctionT,
  DataStream<ExecuteFunctionInput<PipeFunctionT>>,
  ExecuteFunctionOutput<PipeFunctionT>
>;

using SyncNode = sync_node_tag<PipeWithProgress>;

template<class Pass>
using FunctionPassPipe = PipeStream<FunctionPassF<Pass>>;

template<typename Pass>
struct PipeProgressName<FunctionPassF<Pass>> : PipeProgressName<Pass> {
};

template<class PipeFunctionT, bool parallel = true>
using PipeC = pipe_creator_tag<PipeStream<PipeFunctionT>, parallel>;

template<class Pass>
using PassC = pipe_creator_tag<FunctionPassPipe<Pass>>;

template<class PipeFunctionT>
using SyncC = sync_pipe_creator_tag<PipeStream<PipeFunctionT>>;



bool compiler_execute(KphpEnviroment *env) {
  double st = dl_time();
  G = new CompilerCore();
  G->register_env(env);
  G->start();
  if (!env->get_warnings_filename().empty()) {
    FILE *f = fopen(env->get_warnings_filename().c_str(), "w");
    if (!f) {
      std::cerr << "Can't open warnings-file " << env->get_warnings_filename() << "\n";
      return false;
    }

    env->set_warnings_file(f);
  } else {
    env->set_warnings_file(nullptr);
  }
  if (!env->get_stats_filename().empty()) {
    FILE *f = fopen(env->get_stats_filename().c_str(), "w");
    if (!f) {
      std::cerr << "Can't open stats-file " << env->get_stats_filename() << "\n";
      return false;
    }
    env->set_stats_file(f);
  } else {
    env->set_stats_file(nullptr);
  }

  //TODO: call it with pthread_once on need
  lexer_init();
  OpInfo::init_static();
  MultiKey::init_static();
  TypeData::init_static();

//  PhpDocTypeRuleParser::run_tipa_unit_tests_parsing_tags(); return true;

  DataStream<SrcFilePtr> src_file_stream;

  for (const auto &main_file : env->get_main_files()) {
    G->register_main_file(main_file, src_file_stream);
  }
  if (!G->env().get_functions().empty()) {
    G->require_file(G->env().get_functions(), LibPtr{}, src_file_stream);
  }

  static lockf_wrapper unique_file_lock;
  if (!unique_file_lock.lock()) {
    return false;
  }

  SchedulerBase *scheduler;
  if (G->env().get_threads_count() == 1) {
    scheduler = new OneThreadScheduler();
  } else {
    auto s = new Scheduler();
    s->set_threads_count(G->env().get_threads_count());
    scheduler = s;
  }

  G->try_load_tl_classes();

  PipeC<LoadFileF>::get()->set_input_stream(&src_file_stream);

  SchedulerConstructor{scheduler}
    >> PipeC<LoadFileF>{}
    >> PipeC<FileToTokensF>{}
    >> PipeC<ParseF>{}
    >> PassC<GenTreePostprocessPass>{}
    >> PipeC<SplitSwitchF>{}
    >> PipeC<CollectRequiredAndClassesF>{} >> use_nth_output_tag<0>{}
    >> SyncC<CheckRequires>{}
    >> PassC<CalcLocationsPass>{}
    >> PassC<ResolveSelfStaticParentPass>{}
    >> PassC<RegisterDefinesPass>{}
    >> SyncC<CalcRealDefinesValuesF>{}
    >> PassC<EraseDefinesDeclarationsPass>{}
    >> PipeC<PrepareFunctionF>{}
    >> PassC<InlineDefinesUsagesPass>{}
    >> PassC<PreprocessVarargPass>{}
    >> PassC<PreprocessEq3Pass>{}
    >> SyncC<GenerateVirtualMethods>{}
    >> PassC<TransformToSmartInstanceof>{}
    // functions which were generated from templates
    // need to be preprocessed therefore we tie second output and input of Pipe
    >> PipeC<PreprocessFunctionF>{} >> use_nth_output_tag<1>{}
    >> PipeC<PreprocessFunctionF>{} >> use_nth_output_tag<0>{}
    >> PipeC<CalcEmptyFunctions>{}
    >> PassC<CalcActualCallsEdgesPass>{}
    >> SyncC<FilterOnlyActuallyUsedFunctionsF>{}
    >> PassC<RemoveEmptyFunctionCalls>{}
    >> PassC<PreprocessBreakPass>{}
    >> PassC<CalcConstTypePass>{}
    >> PassC<CollectConstVarsPass>{}
    >> PassC<ConvertListAssignmentsPass>{}
    >> PassC<RegisterVariablesPass>{}
    >> PassC<ConvertLocalPhpdocsPass>{}
    >> PassC<CheckFunctionCallsPass>{}
    >> PassC<CheckModificationsOfConstVars>{}
    >> PipeC<CalcRLF>{}
    >> PipeC<CFGBeginF>{}
    >> SyncC<CFGBeginSync>{}
    >> PassC<CloneStrangeConstParams>{}
    >> PassC<CollectMainEdgesPass>{}
    >> SyncC<TypeInfererF>{}
    >> SyncC<CheckRestrictionsF>{}
    >> PipeC<CFGEndF>{}
    >> PassC<CheckClassesPass>{}
    >> PassC<CheckConversionsPass>{}
    >> PassC<OptimizationPass>{}
    >> PassC<FixReturnsPass>{}
    >> PassC<CalcValRefPass>{}
    >> PassC<CalcFuncDepPass>{}
    >> SyncC<CalcBadVarsF>{}
    >> PipeC<CheckUBF>{}
    >> PassC<ExtractResumableCallsPass>{}
    >> PassC<ExtractAsyncPass>{}
    >> PassC<CheckNestedForeachPass>{}
    >> PassC<InlineSimpleFunctions>{}
    >> PassC<CommonAnalyzerPass>{}
    >> PassC<CheckTlClasses>{}
    >> PassC<CheckAccessModifiersPass>{}
    >> PassC<FinalCheckPass>{}
    >> SyncC<CodeGenF>{}
    >> PipeC<WriteFilesF, false>{};

  SchedulerConstructor{scheduler}
    >> PipeC<CollectRequiredAndClassesF>{} >> use_nth_output_tag<1>{}
    >> PipeC<LoadFileF>{};

  SchedulerConstructor{scheduler}
    >> PipeC<CollectRequiredAndClassesF>{} >> use_nth_output_tag<2>{}
    >> PassC<GenTreePostprocessPass>{};

  SchedulerConstructor{scheduler}
    >> PipeC<CollectRequiredAndClassesF>{} >> use_nth_output_tag<3>{}
    // output 1 is used to restart class processing
    >> PipeC<SortAndInheritClassesF>{} >> use_nth_output_tag<1>{}
    >> PipeC<SortAndInheritClassesF>{} >> use_nth_output_tag<0>{}
    >> PassC<GenTreePostprocessPass>{};

  if (G->env().get_show_progress()) {
    PipesProgress::get().enable();
  }

  std::cerr << "Starting php to cpp transpiling...\n";
  get_scheduler()->execute();

  PipesProgress::get().transpiling_process_finish();

  if (G->env().get_error_on_warns() && stage::warnings_count > 0) {
    stage::error();
  }

  stage::die_if_global_errors();
  const int verbosity = G->env().get_verbosity();

  if (verbosity > 0) {
    bool got_changes = false;
    for (const auto &file: G->get_index().get_files()) {
      if (file->is_changed) {
        std::cerr << "\nFile [" << file->path << "] changed";
        got_changes = true;
      }
    }
    if (got_changes) {
      std::cerr << "\n";
    }
  }

  auto profiler_stats = collect_profiler_stats();
  if (verbosity > 1) {
    profiler_print_all(profiler_stats);
  }

  if (G->env().get_use_make()) {
    std::cerr << "\nStarting make...\n";
    run_make();
  }
  const std::string compilation_metrics_file = G->env().get_compilation_metrics_filename();
  G->finish();

  G->stats.update_memory_stats();
  G->stats.total_time = dl_time() - st;
  if (verbosity > 1) {
    std::cerr << std::endl;
    std::cerr << "Compile stats:" << std::endl;
    G->stats.write_to(std::cerr);
  }
  if (!compilation_metrics_file.empty()) {
    G->stats.profiler_stats = std::move(profiler_stats);
    std::ofstream compilation_metrics{compilation_metrics_file};
    G->stats.write_to(compilation_metrics, false);
  }
  return true;
}
