// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/pipes/sync.h"
#include "compiler/scheduler/pipe.h"

template<template<typename, typename, typename> class PipeT>
struct sync_node_tag {
  template<class PipeF, class InputStreamT, class OutputStreamT>
  using PipeType = PipeT<PipeF, InputStreamT, OutputStreamT>;
};

template<size_t id>
struct use_nth_output_tag {};

template<class PipeT, bool parallel, bool unique>
class pipe_creator;

template<class PipeT, bool parallel>
class pipe_creator<PipeT, parallel, true> {
  static PipeT *pipe;

public:
  static PipeT *get() {
    if (pipe == nullptr) {
      pipe = new PipeT(parallel);
    }
    return pipe;
  }
};

template<class PipeT, bool parallel>
PipeT *pipe_creator<PipeT, parallel, true>::pipe = nullptr;

template<class PipeT, bool parallel>
class pipe_creator<PipeT, parallel, false> {
public:
  static PipeT *get() {
    return new PipeT(parallel);
  }
};

template<class PipeT, bool parallel = true, bool unique = true>
struct pipe_creator_tag : pipe_creator<PipeT, parallel, unique> {
  static_assert(!PipeT::pipe_fun_have_on_finish::value, "Non-sync pipe mustn't have on_finish function");
};

template<class PipeT, bool parallel = true, bool unique = true>
struct sync_pipe_creator_tag : pipe_creator<PipeT, parallel, unique> {
  static_assert(PipeT::pipe_fun_have_on_finish::value, "Sync pipe must have on_finish function");
};

template<class StreamT>
class SC_Pipe {
private:
  SchedulerBase *scheduler;
  StreamT *&previous_output_stream;
  Node *previous_node;

private:
  static void connect(StreamT *&first_stream, StreamT *&second_stream) {
    if (first_stream != nullptr && second_stream != nullptr) {
      assert(first_stream == second_stream);
      return;
    }

    if (first_stream != nullptr) {
      second_stream = first_stream;
    } else if (second_stream != nullptr) {
      first_stream = second_stream;
    } else {
      first_stream = new StreamT();
      second_stream = first_stream;
    }
  }

  template<class NextPipeT>
  SC_Pipe<typename NextPipeT::OutputStreamType> operator>>(NextPipeT &next_pipe) {
    connect(previous_output_stream, next_pipe.get_input_stream());
    next_pipe.init_output_stream_if_not_inited();
    return {scheduler, &next_pipe};
  }

public:
  SC_Pipe(SchedulerBase *scheduler, StreamT *&stream, Node *previous_node)
    : scheduler(scheduler)
    , previous_output_stream(stream)
    , previous_node(previous_node) {}

  template<class PipeT>
  SC_Pipe(SchedulerBase *scheduler, PipeT *pipe)
    : scheduler(scheduler)
    , previous_output_stream(pipe->get_output_stream())
    , previous_node(pipe) {
    pipe->add_to_scheduler(scheduler);
  }

  template<template<typename, typename, typename> class PipeT>
  SC_Pipe operator>>(sync_node_tag<PipeT>) {
    using PipeType = typename sync_node_tag<PipeT>::template PipeType<SyncPipeF<typename StreamT::DataType>, StreamT, StreamT>;
    return *this >> sync_pipe_creator_tag<PipeType, true, false>{};
  }

  template<typename NextPipeT, bool parallel, bool unique>
  SC_Pipe<typename NextPipeT::OutputStreamType> operator>>(pipe_creator<NextPipeT, parallel, unique>) {
    auto pipe = pipe_creator<NextPipeT, parallel, unique>::get();
    auto res = *this >> *pipe;
    if (NextPipeT::pipe_fun_have_on_finish::value) {
      pipe->add_to_scheduler_as_sync_node();
    }
    return res;
  }

  template<size_t id>
  SC_Pipe<ConcreteIndexedStream<id, StreamT>> operator>>(use_nth_output_tag<id>) {
    return SC_Pipe<ConcreteIndexedStream<id, StreamT>>{scheduler, previous_output_stream->template project_to_nth_data_stream<id>(), previous_node};
  }
};

class SchedulerConstructor {
  SchedulerBase *scheduler;

public:
  template<typename NextPipeT, bool parallel, bool unique>
  SC_Pipe<typename NextPipeT::OutputStreamType> operator>>(pipe_creator<NextPipeT, parallel, unique>) {
    auto pipe = pipe_creator<NextPipeT, parallel, unique>::get();
    SC_Pipe<typename NextPipeT::OutputStreamType> sc_pipe{scheduler, pipe};
    if (NextPipeT::pipe_fun_have_on_finish::value) {
      pipe->add_to_scheduler_as_sync_node();
    }
    return sc_pipe;
  }

  explicit SchedulerConstructor(SchedulerBase *scheduler)
    : scheduler(scheduler) {}
};
