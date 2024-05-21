// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <forward_list>
#include <mutex>
#include <vector>

#include "common/algorithms/find.h"

#include "compiler/scheduler/scheduler-base.h"
#include "compiler/stage.h"

template<class DataT>
class DataStream {
public:
  using DataType = DataT;
  using StreamType = DataStream<DataType>;

  template<size_t data_id>
  using NthDataType = DataType;

  explicit DataStream(bool is_sink = false)
    : is_sink_mode_(is_sink) {}

  bool get(DataType &result) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!queue_.empty()) {
      result = std::move(queue_.front());
      queue_.pop_front();
      return true;
    }
    return false;
  }

  void operator<<(DataType input) {
    if (!is_sink_mode_) {
      tasks_before_sync_node.fetch_add(1, std::memory_order_release);
    }
    std::lock_guard<std::mutex> lock{mutex_};
    queue_.push_front(std::move(input));
  }

  std::forward_list<DataType> flush() {
    std::lock_guard<std::mutex> lock{mutex_};
    return std::move(queue_);
  }

  std::vector<DataType> flush_as_vector() {
    auto flushed = flush();
    return {std::make_move_iterator(flushed.begin()), std::make_move_iterator(flushed.end())};
  }

private:
  std::mutex mutex_;
  std::forward_list<DataT> queue_;
  const bool is_sink_mode_;
};

struct EmptyStream {
  template<size_t stream_id>
  using NthDataType = EmptyStream;
};

template<class... DataTypes>
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
