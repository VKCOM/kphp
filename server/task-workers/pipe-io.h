// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/param.h>

namespace task_workers {

class PipeIO {
public:
  void reset() {
    buf_end_pos = 0;
    buf_size = -1;
  };

protected:
  int64_t buf[PIPE_BUF / sizeof(int64_t)]{};
  size_t buf_end_pos{0};
  ssize_t buf_size{-1};
};

class PipeTaskWriter : public PipeIO {
public:
  void append(int64_t val);
  bool flush_to_pipe(int write_fd, const char *description);
};

class PipeTaskReader : public PipeIO {
public:
  PipeTaskReader() = default;

  explicit PipeTaskReader(int read_fd)
    : read_fd(read_fd) {}

  ssize_t read_task_from_pipe(size_t task_byte_size);
  ssize_t read_task_results_from_pipe(size_t task_result_byte_size);
  int64_t next();
private:
  int read_fd{-1};
};

} // namespace task_workers
