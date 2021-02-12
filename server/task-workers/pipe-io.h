// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <climits>
#include <cstdint>
#include <type_traits>

#include "server/task-workers/task.h"

namespace task_workers {

class PipeIO {
public:
  void reset() {
    buf_end_pos = 0;
  };

protected:
  unsigned char buf[PIPE_BUF]{};
  size_t buf_end_pos{0};

  PipeIO() = default;

  template <typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
  void copy_to_buffer(const T &x) {
    memcpy(buf + buf_end_pos, &x, sizeof(T));
    buf_end_pos += sizeof(T);
  }

  template <typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
  void extract_from_buffer(T &dest) {
    memcpy(&dest, buf, sizeof(T));
  }
};

class PipeTaskWriter : public PipeIO {
public:
  bool write_task(const Task &task, int write_fd);
  bool write_task_result(const TaskResult &task_result, int write_fd);

private:
  bool write_to_pipe(int write_fd, const char *description);
};

class PipeTaskReader : public PipeIO {
public:
  PipeTaskReader() = default;

  explicit PipeTaskReader(int read_fd)
    : read_fd(read_fd) {}

  enum ReadStatus {
    READ_OK,
    READ_FAIL,
    READ_BLOCK
  };

  ReadStatus read_task(Task &task);
  ReadStatus read_task_result(TaskResult &task_result);
private:
  int read_fd{-1};

  ReadStatus read_from_pipe(size_t bytes_cnt, const char *description);
};

} // namespace task_workers
