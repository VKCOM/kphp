#pragma once

#include "runtime/stream_functions.h"

struct tcp_stream_functions : stream_functions {
  tcp_stream_functions() {
    name = string("tcp");
    supported_functions.set_value(string("fopen"), false);
    supported_functions.set_value(string("fwrite"), true);
    supported_functions.set_value(string("fseek"), false);
    supported_functions.set_value(string("ftell"), false);
    supported_functions.set_value(string("fread"), true);
    supported_functions.set_value(string("fgetc"), true);
    supported_functions.set_value(string("fgets"), true);
    supported_functions.set_value(string("fpassthru"), false);
    supported_functions.set_value(string("fflush"), false);
    supported_functions.set_value(string("feof"), true);
    supported_functions.set_value(string("fclose"), true);
    supported_functions.set_value(string("file_get_contents"), false);
    supported_functions.set_value(string("file_put_contents"), false);
    supported_functions.set_value(string("stream_socket_client"), true);
    supported_functions.set_value(string("context_set_option"), false);
    supported_functions.set_value(string("stream_set_option"), true);
    supported_functions.set_value(string("get_fd"), true);
  }

  ~tcp_stream_functions() override = default;

  Optional<int64_t> fwrite(const Stream &stream, const string &data) const override;

  Optional<string> fread(const Stream &stream, int64_t length) const override;

  Optional<string> fgetc(const Stream &stream) const override;

  Optional<string> fgets(const Stream &stream, int64_t length) const override;

  bool feof(const Stream &stream) const override;

  bool fclose(const Stream &stream) const override;

  Stream stream_socket_client(const string &url, int64_t &error_number, string &error_description, double timeout, int64_t flags, const mixed &context) const override;

  bool stream_set_option(const Stream &stream, int64_t option, int64_t value) const override;

  int32_t get_fd(const Stream &stream) const override;
};

void global_init_tcp_lib();

void free_tcp_lib();
