#pragma once

#include "runtime/kphp_core.h"

using Stream = var;


constexpr int64_t STREAM_SET_BLOCKING_OPTION = 0;
constexpr int64_t STREAM_SET_WRITE_BUFFER_OPTION = 1;
constexpr int64_t STREAM_SET_READ_BUFFER_OPTION = 2;

constexpr int64_t FILE_APPEND = 1;


struct stream_functions {
  string name;

  Stream (*fopen)(const string &stream, const string &mode);

  Stream (*stream_socket_client)(const string &url, int64_t &error_number, string &error_description, double timeout, int64_t flags, const var &context);

  Optional<int64_t> (*fwrite)(const Stream &stream, const string &data);

  int64_t (*fseek)(const Stream &stream, int64_t offset, int64_t whence);

  Optional<int64_t> (*ftell)(const Stream &stream);

  Optional<string> (*fread)(const Stream &stream, int64_t length);

  Optional<string> (*fgetc)(const Stream &stream);

  Optional<string> (*fgets)(const Stream &stream, int64_t length);

  Optional<int64_t> (*fpassthru)(const Stream &stream);

  bool (*fflush)(const Stream &stream);

  bool (*feof)(const Stream &stream);

  bool (*fclose)(const Stream &stream);

  Optional<string> (*file_get_contents)(const string &url);

  Optional<int64_t> (*file_put_contents)(const string &url, const string &content, int64_t flags);

  bool (*context_set_option)(var &context, const string &option, const var &value);

  bool (*stream_set_option)(const Stream &stream, int64_t option, int64_t value);

  int32_t (*get_fd)(const Stream &stream);
};


void register_stream_functions(const stream_functions *functions, bool is_default);


Stream f$fopen(const string &stream, const string &mode);

Optional<int64_t> f$fwrite(const Stream &stream, const string &text);

int64_t f$fseek(const Stream &stream, int64_t offset, int64_t whence = 0);

bool f$rewind(const Stream &stream);

Optional<int64_t> f$ftell(const Stream &stream);

Optional<string> f$fread(const Stream &stream, int64_t length);

Optional<string> f$fgetc(const Stream &stream);

Optional<string> f$fgets(const Stream &stream, int64_t length = -1);

Optional<int64_t> f$fpassthru(const Stream &stream);

bool f$fflush(const Stream &stream);

bool f$feof(const Stream &stream);

bool f$fclose(const Stream &stream);

Optional<int64_t> f$fprintf(const Stream &stream, const string &format, const array<var> &args);

Optional<int64_t> f$vfprintf(const Stream &stream, const string &format, const array<var> &args);

Optional<int64_t> f$fputcsv(const Stream &stream, const array<var> &fields, string delimiter = string(",", 1),
                            string enclosure = string("\"", 1), string escape_char = string("\\", 1));

Optional<array<var>> f$fgetcsv(const Stream &stream, int64_t length = 0, string delimiter = string(",", 1),
                              string enclosure = string("\"", 1), string escape_char = string("\\", 1));

Optional<string> f$file_get_contents(const string &stream);

Optional<int64_t> f$file_put_contents(const string &stream, const var &content_var, int64_t flags = 0);


var f$stream_context_create(const var &options = array<var>());

bool f$stream_context_set_option(var &context, const var &options_var);
bool f$stream_context_set_option(var &context, const var &, const string &);
bool f$stream_context_set_option(var &context, const var &wrapper, const string &option, const var &value);

extern var error_number_dummy;
extern var error_description_dummy;

constexpr int64_t STREAM_CLIENT_CONNECT = 1;

var f$stream_socket_client(const string &url, var &error_number = error_number_dummy, var &error_description = error_description_dummy, double timeout = -1, int64_t flags = STREAM_CLIENT_CONNECT, const var &context = var());


bool f$stream_set_blocking(const Stream &stream, bool mode);

int64_t f$stream_set_write_buffer(const Stream &stream, int64_t size);

int64_t f$stream_set_read_buffer(const Stream &stream, int64_t size);

Optional<int64_t> f$stream_select(var &read, var &write, var &except, const var &tv_sec, int64_t tv_usec = 0);


void init_streams_lib();
void free_streams_lib();
