#pragma once

#include <cassert>
#include <tuple>
#include <unistd.h>
#include <vector>

#include "common/algorithms/find.h"

#include "compiler/scheduler/scheduler-base.h"
#include "compiler/threading/locks.h"

template<class DataT>
class DataStream : Lockable {
private:
  static const int MAX_STREAM_ELEMENTS = 200000;
  DataT *data;
  volatile int *ready;
  volatile int write_i, read_i;
  bool sink;
public:
  using DataType = DataT;
  using StreamType = DataStream<DataT>;

  template<size_t data_id>
  using NthDataType = DataType;

  DataStream() :
    write_i(0),
    read_i(0),
    sink(false) {
    //FIXME
    data = new DataT[MAX_STREAM_ELEMENTS]();
    ready = new int[MAX_STREAM_ELEMENTS]();
  }

  bool empty() {
    return read_i == write_i;
  }

  bool get(DataT &result) {
    while (true) {
      int old_read_i = read_i;
      if (old_read_i < write_i) {
        if (__sync_bool_compare_and_swap(&read_i, old_read_i, old_read_i + 1)) {
          while (!ready[old_read_i]) {
            usleep(250);
          }
          result = std::move(data[old_read_i]);
          return true;
        }
        usleep(250);
      } else {
        return false;
      }
    }
  }

  void operator<<(DataType input) {
    if (!sink) {
      __sync_fetch_and_add(&tasks_before_sync_node, 1);
    }
    while (true) {
      int old_write_i = write_i;
      assert(old_write_i < MAX_STREAM_ELEMENTS);
      if (__sync_bool_compare_and_swap(&write_i, old_write_i, old_write_i + 1)) {
        data[old_write_i] = std::move(input);
        __sync_synchronize();
        ready[old_write_i] = 1;
        return;
      }
      usleep(250);
    }
  }

  int size() {
    return write_i - read_i;
  }

  std::vector<DataT> get_as_vector() {
    return std::vector<DataT>(std::make_move_iterator(data + read_i), std::make_move_iterator(data + write_i));
  }

  void set_sink(bool new_sink) {
    if (new_sink == sink) {
      return;
    }
    sink = new_sink;
    if (sink) {
      __sync_fetch_and_sub(&tasks_before_sync_node, size());
    } else {
      __sync_fetch_and_add(&tasks_before_sync_node, size());
    }
  }
};


struct EmptyStream {
  template<size_t stream_id>
  using NthDataType = EmptyStream;
};

template<class ...DataTypes>
class MultipleDataStreams {
private:
  std::tuple<DataStream<DataTypes> *...> streams_;

public:
  template<size_t data_id>
  using NthDataType = vk::get_nth_type<data_id, DataTypes...>;

  template<size_t stream_id>
  decltype(std::get<stream_id>(streams_)) &project_to_nth_data_stream() {
    return std::get<stream_id>(streams_);
  }

  template<class DataType>
  DataStream<DataType> *&project_to_single_data_stream() {
    constexpr size_t data_id = vk::index_of_type<DataType, DataTypes...>::value;
    return std::get<data_id>(streams_);
  }

  template<class DataType>
  void operator<<(const DataType &data) {
    *project_to_single_data_stream<DataType>() << data;
  }
};

template<size_t id, class StreamT>
using ConcreteIndexedStream = DataStream<typename StreamT::template NthDataType<id>>;
