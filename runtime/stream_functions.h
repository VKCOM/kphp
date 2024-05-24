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

  virtual Stream fopen([[maybe_unused]] const string &stream, [[maybe_unused]] const string &mode) const {
    php_critical_error("Function \"fopen\" is not supported for %s", name.c_str());
  }

  virtual Optional<int64_t> fwrite([[maybe_unused]] const Stream &stream, [[maybe_unused]] const string &data) const {
    php_critical_error("Function \"fwrite\" is not supported for %s", name.c_str());
  }

  virtual int64_t fseek([[maybe_unused]] const Stream &stream, [[maybe_unused]] int64_t offset, [[maybe_unused]] int64_t whence) const {
    php_critical_error("Function \"Stream\" is not supported for %s", name.c_str());
  }

  virtual Optional<int64_t> ftell([[maybe_unused]] const Stream &stream) const {
    php_critical_error("Function \"ftell\" is not supported for %s", name.c_str());
  }

  virtual Optional<string> fread([[maybe_unused]] const Stream &stream, [[maybe_unused]] int64_t length) const {
    php_critical_error("Function \"fread\" is not supported for %s", name.c_str());
  }

  virtual Optional<string> fgetc([[maybe_unused]] const Stream &stream) const {
    php_critical_error("Function \"fgetc\" is not supported for %s", name.c_str());
  }

  virtual Optional<string> fgets([[maybe_unused]] const Stream &stream, [[maybe_unused]] int64_t length) const {
    php_critical_error("Function \"fgets\" is not supported for %s", name.c_str());
  }

  virtual Optional<int64_t> fpassthru([[maybe_unused]] const Stream &stream) const {
    php_critical_error("Function \"fpassthru\" is not supported for %s", name.c_str());
  }

  virtual bool fflush([[maybe_unused]] const Stream &stream) const {
    php_critical_error("Function \"fflush\" is not supported for %s", name.c_str());
  }

  virtual bool feof([[maybe_unused]] const Stream &stream) const {
    php_critical_error("Function \"feof\" is not supported for %s", name.c_str());
  }

  virtual bool fclose([[maybe_unused]] const Stream &stream) const {
    php_critical_error("Function \"fclose\" is not supported for %s", name.c_str());
  }

  virtual Optional<string> file_get_contents([[maybe_unused]] const string &url) const {
    php_critical_error("Function \"file_get_contents\" is not supported for %s", name.c_str());
  }

  virtual Optional<int64_t> file_put_contents([[maybe_unused]] const string &url, [[maybe_unused]] const string &content, [[maybe_unused]] int64_t flags) const {
    php_critical_error("Function \"file_put_contents\" is not supported for %s", name.c_str());
  }

  virtual Stream stream_socket_client([[maybe_unused]] const string &url, [[maybe_unused]] int64_t &error_number, [[maybe_unused]] string &error_description,
                                      [[maybe_unused]] double timeout, [[maybe_unused]] int64_t flags, [[maybe_unused]] const mixed &context) const {
    php_critical_error("Function \"stream_socket_client\" is not supported for %s", name.c_str());
  }

  virtual bool context_set_option([[maybe_unused]] mixed &context, [[maybe_unused]] const string &option, [[maybe_unused]] const mixed &value) const {
    php_critical_error("Function \"context_set_option\" is not supported for %s", name.c_str());
  }

  virtual bool stream_set_option([[maybe_unused]] const Stream &stream, [[maybe_unused]] int64_t option, [[maybe_unused]] int64_t value) const {
    php_critical_error("Function \"stream_set_option\" is not supported for %s", name.c_str());
  }

  virtual int32_t get_fd([[maybe_unused]] const Stream &stream) const {
    php_critical_error("Function \"get_fd\" is not supported for %s", name.c_str());
  }
};
