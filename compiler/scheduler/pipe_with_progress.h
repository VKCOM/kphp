// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/scheduler/pipe.h"

#include <iomanip>
#include <cmath>

template<typename PipeT>
struct PipeProgressName {
  static std::string get() {
    return demangle(typeid(PipeT).name());
  }
};

struct TranspilingProcessFinishTag {};

template<>
struct PipeProgressName<TranspilingProcessFinishTag> {
  static std::string get() {
    return "Transpiling process finished";
  }
};

class PipesProgress {
public:
  static PipesProgress &get() {
    static PipesProgress this_;
    return this_;
  }

  template<typename PipeFunctionT>
  void on_pipe_process_input() {
    if (enabled_ && is_first_process_input<PipeFunctionT>()) {
      write_progress<PipeFunctionT>(" started");
    }
  }

  template<typename PipeFunctionT>
  void on_pipe_finish() {
    if (enabled_) {
      write_progress<PipeFunctionT>(" finished");
    }
  }

  void transpiling_process_finish() {
    if (enabled_) {
      write_progress<TranspilingProcessFinishTag>();
    }
  }

  void register_stage() {
    std::lock_guard<std::mutex> lock{mutex_};
    ++total_stages_;
  }

  void enable() {
    enabled_ = true;
  }

private:
  template<typename PipeFunctionT>
  void write_progress(const char *name_postfix = "") {
    std::lock_guard<std::mutex> lock{mutex_};
    const size_t percent = *name_postfix ? 100 * ++stages_counter_ / total_stages_ : 100;
    std::cerr << "[" << std::setw(3) << percent << "%] "
              << PipeProgressName<PipeFunctionT>::get() << name_postfix << "\n";
  }

  template<typename PipeFunctionT>
  static bool is_first_process_input() {
    static std::atomic_flag is_first = ATOMIC_FLAG_INIT;
    return is_first.test_and_set() == false;
  }

  std::mutex mutex_;
  size_t total_stages_{1};
  size_t stages_counter_{0};
  std::atomic<bool> enabled_{false};
};

template<class PipeF, class InputStreamT, class OutputStreamT>
class PipeWithProgress final : public Pipe<PipeF, InputStreamT, OutputStreamT> {
private:
  using BasePipe = Pipe<PipeF, InputStreamT, OutputStreamT>;

public:
  using BasePipe::BasePipe;

  explicit PipeWithProgress(bool parallel = true) :
    BasePipe(parallel) {
    PipesProgress::get().register_stage();
    if (BasePipe::pipe_fun_have_on_finish::value) {
      PipesProgress::get().register_stage();
    }
  }

  void process_input(typename BasePipe::InputType &&input) final {
    PipesProgress::get().on_pipe_process_input<PipeF>();
    BasePipe::process_input(std::move(input));
  }

  void on_finish() final {
    BasePipe::on_finish();
    PipesProgress::get().on_pipe_finish<PipeF>();
  }
};
