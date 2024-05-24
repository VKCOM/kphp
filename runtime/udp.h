// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/stream_functions.h"

struct udp_stream_functions : stream_functions {
  udp_stream_functions() {
    name = string("udp");
    supported_functions.set_value(string("fopen"), false);
    supported_functions.set_value(string("fwrite"), true);
    supported_functions.set_value(string("fseek"), false);
    supported_functions.set_value(string("ftell"), false);
    supported_functions.set_value(string("fread"), false);
    supported_functions.set_value(string("fgetc"), false);
    supported_functions.set_value(string("fgets"), false);
    supported_functions.set_value(string("fpassthru"), false);
    supported_functions.set_value(string("fflush"), false);
    supported_functions.set_value(string("feof"), false);
    supported_functions.set_value(string("fclose"), true);
    supported_functions.set_value(string("file_get_contents"), false);
    supported_functions.set_value(string("file_put_contents"), false);
    supported_functions.set_value(string("stream_socket_client"), true);
    supported_functions.set_value(string("context_set_option"), false);
    supported_functions.set_value(string("stream_set_option"), false);
    supported_functions.set_value(string("get_fd"), true);
  }

  ~udp_stream_functions() override = default;

  Optional<int64_t> fwrite(const Stream &stream, const string &data) const override;

  bool fclose(const Stream &stream) const override;

  Stream stream_socket_client(const string &url, int64_t &error_number, string &error_description, double timeout, int64_t flags, const mixed &context) const override;

  int32_t get_fd(const Stream &stream) const override;
};

void global_init_udp_lib();

void free_udp_lib();
