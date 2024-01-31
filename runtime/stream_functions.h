// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

using Stream = mixed;

constexpr int64_t STREAM_SET_BLOCKING_OPTION = 0;
constexpr int64_t STREAM_SET_WRITE_BUFFER_OPTION = 1;
constexpr int64_t STREAM_SET_READ_BUFFER_OPTION = 2;

constexpr int64_t FILE_APPEND = 1;

struct stream_functions {
  stream_functions() = default;
  virtual ~stream_functions() = default;

  string name;
  array<bool> supported_functions;

  virtual Stream fopen(const string &stream, const string &mode) const {
    php_critical_error("Function \"fopen\" is not supported for %s", name.c_str());
  }

  virtual Optional<int64_t> fwrite(const Stream &stream, const string &data) const {
    php_critical_error("Function \"fwrite\" is not supported for %s", name.c_str());
  }

  virtual int64_t fseek(const Stream &stream, int64_t offset, int64_t whence) const {
    php_critical_error("Function \"Stream\" is not supported for %s", name.c_str());
  }

  virtual Optional<int64_t> ftell(const Stream &stream) const {
    php_critical_error("Function \"ftell\" is not supported for %s", name.c_str());
  }

  virtual Optional<string> fread(const Stream &stream, int64_t length) const {
    php_critical_error("Function \"fread\" is not supported for %s", name.c_str());
  }

  virtual Optional<string> fgetc(const Stream &stream) const {
    php_critical_error("Function \"fgetc\" is not supported for %s", name.c_str());
  }

  virtual Optional<string> fgets(const Stream &stream, int64_t length) const {
    php_critical_error("Function \"fgets\" is not supported for %s", name.c_str());
  }

  virtual Optional<int64_t> fpassthru(const Stream &stream) const {
    php_critical_error("Function \"fpassthru\" is not supported for %s", name.c_str());
  }

  virtual bool fflush(const Stream &stream) const {
    php_critical_error("Function \"fflush\" is not supported for %s", name.c_str());
  }

  virtual bool feof(const Stream &stream) const {
    php_critical_error("Function \"feof\" is not supported for %s", name.c_str());
  }

  virtual bool fclose(const Stream &stream) const {
    php_critical_error("Function \"fclose\" is not supported for %s", name.c_str());
  }

  virtual Optional<string> file_get_contents(const string &url) const {
    php_critical_error("Function \"file_get_contents\" is not supported for %s", name.c_str());
  }

  virtual Optional<int64_t> file_put_contents(const string &url, const string &content, int64_t flags) const {
    php_critical_error("Function \"file_put_contents\" is not supported for %s", name.c_str());
  }

  virtual Stream stream_socket_client(const string &url, int64_t &error_number, string &error_description, double timeout, int64_t flags, const mixed &context) const {
    php_critical_error("Function \"stream_socket_client\" is not supported for %s", name.c_str());
  }

  virtual bool context_set_option(mixed &context, const string &option, const mixed &value) const {
    php_critical_error("Function \"context_set_option\" is not supported for %s", name.c_str());
  }

  virtual bool stream_set_option(const Stream &stream, int64_t option, int64_t value) const {
    php_critical_error("Function \"stream_set_option\" is not supported for %s", name.c_str());
  }

  virtual int32_t get_fd(const Stream &stream) const {
    php_critical_error("Function \"get_fd\" is not supported for %s", name.c_str());
  }
};
