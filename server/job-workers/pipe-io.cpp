// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstring>
#include <cerrno>
#include <unistd.h>

#include "common/kprintf.h"
#include "server/job-workers/pipe-io.h"

namespace job_workers {

bool PipeJobWriter::write_to_pipe(int write_fd, const char *description) {
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

bool PipeJobWriter::write_job(const Job &job, int write_fd) {
  reset();
  copy_to_buffer(job);
  return write_to_pipe(write_fd, "writing job");
}

bool PipeJobWriter::write_job_result(const JobResult &job_result, int write_fd) {
  reset();
  copy_to_buffer(job_result);
  return write_to_pipe(write_fd, "writing result of job");
}

PipeJobReader::ReadStatus PipeJobReader::read_job(Job &job) {
  reset();
  ReadStatus status = read_from_pipe(sizeof(Job), "read job");
  if (status == READ_OK) {
    extract_from_buffer(job);
  }
  return status;
}

PipeJobReader::ReadStatus PipeJobReader::read_job_result(JobResult &job_result) {
  reset();
  ReadStatus status = read_from_pipe(sizeof(JobResult), "read result of job");
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
    kprintf("Couldn't %s: %s\n", description, strerror(errno));
    return READ_FAIL;
  }
  if (read_bytes != bytes_cnt) {
    kprintf("Couldn't %s. Got %zd bytes, but expected %zd. Probably some jobs are written not atomically\n", description, bytes_cnt, read_bytes);
    return READ_FAIL;
  }
  return READ_OK;
}

} // namespace job_workers
