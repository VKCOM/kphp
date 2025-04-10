// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <typeinfo>

#include "compiler/scheduler/node.h"
#include "compiler/scheduler/task.h"
#include "compiler/threading/profiler.h"

template<class T, class Enable = void>
struct NeedProfiler : std::true_type {};

template<class T>
struct NeedProfiler<T, typename std::enable_if<!T::need_profiler::value>::type> : std::false_type {};

template<class T, class Enable = void>
struct NeedOnFinishProfiler : std::true_type {};

template<class T>
struct NeedOnFinishProfiler<T, typename std::enable_if<!T::need_on_finish_profiler::value>::type> : std::false_type {};

template<class PipeType>
class PipeTask : public Task {
private:
  using InputType = typename PipeType::InputType;
  InputType input;
  PipeType* pipe_ptr;
  static ProfilerRaw& get_task_profiler() {
    static CachedProfiler cache{demangle(typeid(typename PipeType::PipeFunctionType).name())};
    return *cache;
  }

public:
  PipeTask(InputType&& input, PipeType* pipe_ptr)
      : input(std::move(input)),
        pipe_ptr(pipe_ptr) {}

  void execute() override {
    if (NeedProfiler<typename PipeType::PipeFunctionType>::value) {
      AutoProfiler prof{get_task_profiler()};
      pipe_ptr->process_input(std::move(input));
    } else {
      pipe_ptr->process_input(std::move(input));
    }
  }
};

template<class PipeF, class InputStreamT, class OutputStreamT>
class Pipe : public Node {
  struct have_on_finish_helper {
    template<typename T, void (T::*)(OutputStreamT&)>
    struct SFINAE;
    template<typename T>
    static std::true_type Test(SFINAE<T, &T::on_finish>*);
    template<typename T>
    static std::false_type Test(...);
  };

public:
  using PipeFunctionType = PipeF;
  using InputStreamType = InputStreamT;
  using OutputStreamType = OutputStreamT;

  using InputType = typename InputStreamT::DataType;
  using SelfType = Pipe<PipeF, InputStreamT, OutputStreamT>;
  using TaskType = PipeTask<SelfType>;

  using pipe_fun_have_on_finish = decltype(have_on_finish_helper::template Test<PipeF>(0));

private:
  InputStreamType* input_stream = nullptr;
  OutputStreamType* output_stream = nullptr;
  PipeF function;

  void on_finish(std::true_type) {
    if (NeedOnFinishProfiler<PipeF>::value) {
      AutoProfiler prof{get_profiler(demangle(typeid(PipeF).name()) + "::on_finish")};
      function.on_finish(*output_stream);
    } else {
      function.on_finish(*output_stream);
    }
  }

  void on_finish(std::false_type) {}

public:
  explicit Pipe(bool parallel = true)
      : Node(parallel) {}

  InputStreamType*& get_input_stream() {
    return input_stream;
  }

  OutputStreamType*& get_output_stream() {
    return output_stream;
  }

  void init_output_stream_if_not_inited() {
    if (!output_stream) {
      output_stream = new OutputStreamType();
    }
  }

  void set_input_stream(InputStreamType* is) {
    input_stream = is;
  }

  Task* get_task() override {
    InputType x{};
    if (!input_stream->get(x)) {
      return nullptr;
    }
    return new TaskType(std::move(x), this);
  }

  virtual void process_input(InputType&& input) {
    function.execute(std::move(input), *output_stream);
  }

  void on_finish() override {
    on_finish(pipe_fun_have_on_finish{});
  }
};
