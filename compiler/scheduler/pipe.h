#pragma once

#include "compiler/bicycle.h"
#include "compiler/scheduler/node.h"
#include "compiler/scheduler/task.h"

template<class PipeType>
class PipeTask : public Task {
private:
  typedef typename PipeType::InputType InputType;
  InputType input;
  PipeType *pipe_ptr;
public:
  PipeTask(InputType input, PipeType *pipe_ptr) :
    input(input),
    pipe_ptr(pipe_ptr) {
  }

  void execute() override {
    pipe_ptr->process_input(input);
  }
};

template<class PipeF, class InputStreamT, class OutputStreamT>
class Pipe : public Node {
  struct have_on_finish_helper {
    template<typename T, void (T::*)(OutputStreamT &)> struct SFINAE;
    template<typename T> static std::true_type Test(SFINAE<T, &T::on_finish> *);
    template<typename T> static std::false_type Test(...);
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
  InputStreamType *input_stream = nullptr;
  OutputStreamType *output_stream = nullptr;
  PipeF function;

public:
  explicit Pipe(bool parallel = true) :
    Node(parallel) {}

  InputStreamType *&get_input_stream() { return input_stream; }

  OutputStreamType *&get_output_stream() { return output_stream; }

  void init_output_stream_if_not_inited() {
    if (!output_stream) {
      output_stream = new OutputStreamType();
    }
  }

  void set_input_stream(InputStreamType *is) { input_stream = is; }

  void process_input(const InputType &input) { function.execute(input, *output_stream); }

  Task *get_task() override {
    Maybe<InputType> x = input_stream->get();
    if (x.empty()) {
      return nullptr;
    }
    return new TaskType(x, this);
  }

  void on_finish(std::true_type) { function.on_finish(*output_stream); }
  void on_finish(std::false_type) {}
  void on_finish() override { on_finish(pipe_fun_have_on_finish{}); }
};
