#include <cassert>
#include <cstring>
#include <errno.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "server/task-workers/pipe-io.h"

namespace task_workers {

bool PipeTaskWriter::flush_to_pipe(int write_fd, const char *description) {
  size_t bytes_to_write = buf_end_pos * sizeof(buf[0]);
  ssize_t written = write(write_fd, buf, bytes_to_write);

  if (written == -1) {
    if (errno == EWOULDBLOCK) {
      kprintf("Fail on %s to pipe: pipe is full\n", description);
    } else {
      kprintf("Fail on %s to pipe: %s\n", description, strerror(errno));
    }
    return false;
  }

  if (written != bytes_to_write) {
    kprintf("Fail on %s to pipe: written bytes = %zd, but requested = %zd\n", description, written, bytes_to_write);
    return false;
  }

  buf_end_pos = 0;
  return true;
}

void PipeTaskWriter::append(int64_t val) {
  buf[buf_end_pos++] = val;
}

ssize_t PipeTaskReader::read_task_from_pipe(size_t task_byte_size) {
  ssize_t read_bytes = read(read_fd, buf, task_byte_size);
  if (read_bytes == -1) {
    if (errno == EWOULDBLOCK) {
      return -2;
    }
    kprintf("pid = %d, Couldn't read task: %s\n", getpid(), strerror(errno));
    return -1;
  }
  if (read_bytes != task_byte_size) {
    kprintf("Couldn't read task. Got %zd bytes, but task bytes size = %zd. Probably some tasks are written not atomically\n", read_bytes, task_byte_size);
    return -1;
  }
  buf_size = read_bytes / sizeof(buf[0]);
  return read_bytes;
}

ssize_t PipeTaskReader::read_task_results_from_pipe(size_t task_result_byte_size) {
  ssize_t read_bytes = read(read_fd, buf, PIPE_BUF);

  if (read_bytes == -1) {
    assert(errno != EWOULDBLOCK); // because fd must be ready for read, current HTTP worker is only one reader for this fd
    kprintf("Couldn't read task results: %s", strerror(errno));
    return -1;
  }
  if (read_bytes % task_result_byte_size != 0) {
    kprintf("Got inconsistent number of bytes on read task results: %zd. Probably some task results are written not atomically", read_bytes);
    return -1;
  }
  buf_size = read_bytes / sizeof(buf[0]);
  return read_bytes;
}

int64_t PipeTaskReader::next() {
  assert(buf_end_pos < buf_size);
  return buf[buf_end_pos++];
}

} // namespace task_workers
