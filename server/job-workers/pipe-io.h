// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <climits>
#include <cstdint>
#include <type_traits>

namespace job_workers {

struct JobSharedMessage;

class PipeIO {
public:
  void reset() {
    buf_end_pos = 0;
  };

protected:
  unsigned char buf[PIPE_BUF]{};
  size_t buf_end_pos{0};

  PipeIO() = default;

  template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
  void copy_to_buffer(T x) {
    memcpy(buf + buf_end_pos, &x, sizeof(T));
    buf_end_pos += sizeof(T);
  }

  template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
  void extract_from_buffer(T &dest) {
    memcpy(&dest, buf, sizeof(T));
  }
};

class PipeJobWriter : public PipeIO {
public:
  bool write_job(JobSharedMessage *job, int write_fd);
  bool write_job_result(JobSharedMessage *job_result, int write_fd);

private:
  bool write_to_pipe(int write_fd, const char *description);
};

class PipeJobReader : public PipeIO {
public:
  PipeJobReader() = default;

  explicit PipeJobReader(int read_fd)
    : read_fd(read_fd) {}

  enum ReadStatus {
    READ_OK,
    READ_FAIL,
    READ_BLOCK
  };

  ReadStatus read_job(JobSharedMessage *&job);
  ReadStatus read_job_result(JobSharedMessage *&job_result);
private:
  int read_fd{-1};

  ReadStatus read_from_pipe(size_t bytes_cnt, const char *description);
};

} // namespace job_workers
