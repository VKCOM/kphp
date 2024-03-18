// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include "common/kprintf.h"
#include "server/job-workers/pipe-io.h"
#include "server/server-log.h"

namespace job_workers {

bool PipeJobWriter::write_to_pipe(int write_fd, const char *description) {
  size_t bytes_to_write = buf_end_pos;
  ssize_t written = write(write_fd, buf, bytes_to_write);

  if (written == -1) {
    if (errno == EWOULDBLOCK) {
      log_server_error("Fail on %s to pipe: pipe is full", description);
    } else {
      log_server_error("Fail on %s to pipe: %s", description, strerror(errno));
    }
    return false;
  }

  if (written != bytes_to_write) {
    log_server_error("Fail on %s to pipe: written bytes = %zd, but requested = %zd", description, written, bytes_to_write);
    return false;
  }

  buf_end_pos = 0;
  return true;
}

bool PipeJobWriter::write_job(JobSharedMessage *job, int write_fd) {
  reset();
  copy_to_buffer(job);
  return write_to_pipe(write_fd, "writing job");
}

bool PipeJobWriter::write_job_result(JobSharedMessage *job_result, int write_fd) {
  reset();
  copy_to_buffer(job_result);
  return write_to_pipe(write_fd, "writing result of job");
}

PipeJobReader::ReadStatus PipeJobReader::read_job(JobSharedMessage *&job) {
  reset();
  ReadStatus status = read_from_pipe(sizeof(JobSharedMessage *), "read job");
  if (status == READ_OK) {
    extract_from_buffer(job);
  }
  return status;
}

PipeJobReader::ReadStatus PipeJobReader::read_job_result(JobSharedMessage *&job_result) {
  reset();
  ReadStatus status = read_from_pipe(sizeof(JobSharedMessage *), "read result of job");
  if (status == READ_OK) {
    extract_from_buffer(job_result);
  }
  return status;
}

PipeJobReader::ReadStatus PipeJobReader::read_from_pipe(size_t bytes_cnt, const char *description) {
  ssize_t read_bytes = read(read_fd, buf, bytes_cnt);
  if (read_bytes == -1) {
    if (errno == EWOULDBLOCK) {
      return READ_BLOCK;
    }
    log_server_error("Couldn't %s: %s", description, strerror(errno));
    return READ_FAIL;
  }
  if (read_bytes != bytes_cnt) {
    log_server_error("Couldn't %s. Got %zd bytes, but expected %zd. Probably some jobs are written not atomically", description, bytes_cnt, read_bytes);
    return READ_FAIL;
  }
  return READ_OK;
}

} // namespace job_workers
