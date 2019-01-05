#include "compiler/compiler.h"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <functional>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

#include "common/crc32.h"
#include "common/type_traits/function_traits.h"
#include "common/version-string.h"

#include "compiler/compiler-core.h"
#include "compiler/const-manipulations.h"
#include "compiler/data/data_ptr.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/io.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
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
#include "compiler/pipes/check-function-calls.h"
#include "compiler/pipes/check-infered-instances.h"
#include "compiler/pipes/check-instance-props.h"
#include "compiler/pipes/check-nested-foreach.h"
#include "compiler/pipes/check-returns.h"
#include "compiler/pipes/check-ub.h"
#include "compiler/pipes/code-gen.h"
#include "compiler/pipes/collect-const-vars.h"
#include "compiler/pipes/collect-required-and-classes.h"
#include "compiler/pipes/convert-list-assignments.h"
#include "compiler/pipes/create-switch-foreach-vars.h"
#include "compiler/pipes/erase-defines-declarations.h"
#include "compiler/pipes/extract-async.h"
#include "compiler/pipes/extract-resumable-calls.h"
#include "compiler/pipes/file-and-token.h"
#include "compiler/pipes/file-to-tokens.h"
#include "compiler/pipes/filter-only-actually-used.h"
#include "compiler/pipes/final-check.h"
#include "compiler/pipes/gen-tree-postprocess.h"
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
#include "compiler/pipes/type-inferer-end.h"
#include "compiler/pipes/type-inferer.h"
#include "compiler/pipes/write-files.h"
#include "compiler/scheduler/constructor.h"
#include "compiler/scheduler/one-thread-scheduler.h"
#include "compiler/scheduler/pipe.h"
#include "compiler/scheduler/scheduler.h"
#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

class lockf_wrapper {
  std::string locked_filename_;
  int fd_ = -1;
  bool locked_ = false;

  void close_and_unlink() {
    if (close(fd_) == -1) {
      perror(("Can't close file: " + locked_filename_).c_str());
      return;
    }

    if (unlink(locked_filename_.c_str()) == -1) {
      perror(("Can't unlink file: " + locked_filename_).c_str());
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
    ss << "/tmp/" << std::hex << hash(dest_path) << "_kphp_temp_lock";
    locked_filename_ = ss.str();

    fd_ = open(locked_filename_.c_str(), O_RDWR | O_CREAT, 0666);
    assert(fd_ != -1);

    locked_ = true;
    if (lockf(fd_, F_TLOCK, 0) == -1) {
      perror("\nCan't lock file, maybe you have already run kphp2cpp");
      fprintf(stderr, "\n");

      close(fd_);
      locked_ = false;
    }

    return locked_;
  }

  ~lockf_wrapper() {
    if (locked_) {
      if (lockf(fd_, F_ULOCK, 0) == -1) {
        perror(("Can't unlock file: " + locked_filename_).c_str());
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
using PipeStream = Pipe<
  PipeFunctionT,
  DataStream<ExecuteFunctionInput<PipeFunctionT>>,
  ExecuteFunctionOutput<PipeFunctionT>
>;

template<class Pass>
using FunctionPassPipe = PipeStream<FunctionPassF<Pass>>;

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
      fprintf(stderr, "Can't open warnings-file %s\n", env->get_warnings_filename().c_str());
      return false;
    }

    env->set_warnings_file(f);
  } else {
    env->set_warnings_file(nullptr);
  }
  if (!env->get_stats_filename().empty()) {
    FILE *f = fopen(env->get_stats_filename().c_str(), "w");
    if (!f) {
      fprintf(stderr, "Can't open stats-file %s\n", env->get_stats_filename().c_str());
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

  DataStream<SrcFilePtr> file_stream;

  for (const auto &main_file : env->get_main_files()) {
    G->register_main_file(main_file, file_stream);
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

  PipeC<LoadFileF>::get()->set_input_stream(&file_stream);


  SchedulerConstructor{scheduler}
    >> PipeC<LoadFileF>{}
    >> PipeC<FileToTokensF>{}
    >> PipeC<ParseF>{}
    >> PassC<GenTreePostprocessPass>{}
    >> PipeC<SplitSwitchF>{}
    >> PassC<CreateSwitchForeachVarsPass>{}
    >> PipeC<CollectRequiredAndClassesF>{} >> use_nth_output_tag<0>{}
    >> sync_node_tag{}
    >> PassC<CalcLocationsPass>{}
    >> PassC<ResolveSelfStaticParentPass>{}
    >> PassC<RegisterDefinesPass>{}
    >> SyncC<CalcRealDefinesValuesF>{}
    >> PassC<EraseDefinesDeclarationsPass>{}
    >> PipeC<PrepareFunctionF>{}
    >> PassC<InlineDefinesUsagesPass>{}
    >> PassC<PreprocessVarargPass>{}
    >> PassC<PreprocessEq3Pass>{}
    >> sync_node_tag{}
    // functions which were generated from templates
    // need to be preprocessed therefore we tie second output and input of Pipe
    >> PipeC<PreprocessFunctionF>{} >> use_nth_output_tag<1>{}
    >> PipeC<PreprocessFunctionF>{} >> use_nth_output_tag<0>{}
    >> PipeC<CalcEmptyFunctions>{}
    >> PipeC<CalcActualCallsEdgesF>{}
    >> SyncC<FilterOnlyActuallyUsedFunctionsF>{}
    >> PassC<RemoveEmptyFunctionCalls>{}
    >> PassC<PreprocessBreakPass>{}
    >> PassC<CalcConstTypePass>{}
    >> PassC<CollectConstVarsPass>{}
    >> PassC<CheckInstancePropsPass>{}
    >> PassC<ConvertListAssignmentsPass>{}
    >> PassC<RegisterVariablesPass>{}
    >> PassC<CheckFunctionCallsPass>{}
    >> PipeC<CalcRLF>{}
    >> PipeC<CFGBeginF>{}
    >> PipeC<CheckReturnsF>{}
    >> sync_node_tag{}
    >> SyncC<TypeInfererF>{}
    >> SyncC<TypeInfererEndF>{}
    >> PipeC<CFGEndF>{}
    >> PipeC<CheckInferredInstancesF>{}
    >> PipeC<CheckClassesF>{}
    >> PassC<OptimizationPass>{}
    >> PassC<CalcValRefPass>{}
    >> SyncC<CalcBadVarsF>{}
    >> PipeC<CheckUBF>{}
    >> PassC<ExtractResumableCallsPass>{}
    >> PassC<ExtractAsyncPass>{}
    >> PassC<CheckNestedForeachPass>{}
    >> PassC<CommonAnalyzerPass>{}
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

  get_scheduler()->execute();

  if (G->env().get_error_on_warns() && stage::warnings_count > 0) {
    stage::error();
  }

  stage::die_if_global_errors();
  int verbosity = G->env().get_verbosity();
  if (G->env().get_use_make()) {
    fprintf(stderr, "start make\n");
    G->make();
  }
  G->finish();
  if (verbosity > 1) {
    profiler_print_all();
    double en = dl_time();
    double passed = en - st;
    fprintf(stderr, "PASSED: %lf\n", passed);
    mem_info_t mem_info;
    get_mem_stats(getpid(), &mem_info);
    fprintf(stderr, "RSS: %lluKb\n", mem_info.rss);
  }
  return true;
}
