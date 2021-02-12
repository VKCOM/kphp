#include <cstring>
#include <errno.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "server/task-workers/pipe-io.h"

namespace task_workers {

bool PipeTaskWriter::write_to_pipe(int write_fd, const char *description) {
  size_t bytes_to_write = buf_end_pos;
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

bool PipeTaskWriter::write_task(const Task &task, int write_fd) {
  reset();
  copy_to_buffer(task);
  return write_to_pipe(write_fd, "writing task");
}

bool PipeTaskWriter::write_task_result(const TaskResult &task_result, int write_fd) {
  reset();
  copy_to_buffer(task_result);
  return write_to_pipe(write_fd, "writing result of task");
}

PipeTaskReader::ReadStatus PipeTaskReader::read_task(Task &task) {
  reset();
  ReadStatus status = read_from_pipe(sizeof(Task), "read task");
  if (status == READ_OK) {
    extract_from_buffer(task);
  }
  return status;
}

PipeTaskReader::ReadStatus PipeTaskReader::read_task_result(TaskResult &task_result) {
  reset();
  ReadStatus status = read_from_pipe(sizeof(TaskResult), "read result of task");
  if (status == READ_OK) {
    extract_from_buffer(task_result);
  }
  return status;
}

PipeTaskReader::ReadStatus PipeTaskReader::read_from_pipe(size_t bytes_cnt, const char *description) {
  ssize_t read_bytes = read(read_fd, buf, bytes_cnt);
  if (read_bytes == -1) {
    if (errno == EWOULDBLOCK) {
      return READ_BLOCK;
    }
    kprintf("Couldn't %s: %s\n", description, strerror(errno));
    return READ_FAIL;
  }
  if (read_bytes != bytes_cnt) {
    kprintf("Couldn't %s. Got %zd bytes, but expected %zd. Probably some tasks are written not atomically\n", description, bytes_cnt, read_bytes);
    return READ_FAIL;
  }
  return READ_OK;
}

} // namespace task_workers
