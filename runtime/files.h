// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "runtime/stream_functions.h"

extern const string LETTER_a;

struct file_stream_functions : stream_functions {
  file_stream_functions() {
    name = string("file");
    supported_functions.set_value(string("fopen"), true);
    supported_functions.set_value(string("fwrite"), true);
    supported_functions.set_value(string("fseek"), true);
    supported_functions.set_value(string("ftell"), true);
    supported_functions.set_value(string("fread"), true);
    supported_functions.set_value(string("fgetc"), true);
    supported_functions.set_value(string("fgets"), true);
    supported_functions.set_value(string("fpassthru"), true);
    supported_functions.set_value(string("fflush"), true);
    supported_functions.set_value(string("feof"), true);
    supported_functions.set_value(string("fclose"), true);
    supported_functions.set_value(string("file_get_contents"), true);
    supported_functions.set_value(string("file_put_contents"), true);
    supported_functions.set_value(string("stream_socket_client"), false);
    supported_functions.set_value(string("context_set_option"), false);
    supported_functions.set_value(string("stream_set_option"), false);
    supported_functions.set_value(string("get_fd"), false);
  }

  ~file_stream_functions() override = default;

  Stream fopen(const string &stream, const string &mode) const override;

  Optional<int64_t> fwrite(const Stream &stream, const string &data) const override;

  int64_t fseek(const Stream &stream, int64_t offset, int64_t whence) const override;

  Optional<int64_t> ftell(const Stream &stream) const override;

  Optional<string> fread(const Stream &stream, int64_t length) const override;

  Optional<string> fgetc(const Stream &stream) const override;

  Optional<string> fgets(const Stream &stream, int64_t length) const override;

  Optional<int64_t> fpassthru(const Stream &stream) const override;

  bool fflush(const Stream &stream) const override;

  bool feof(const Stream &stream) const override;

  bool fclose(const Stream &stream) const override;

  Optional<string> file_get_contents(const string &url) const override;

  Optional<int64_t> file_put_contents(const string &url, const string &content, int64_t flags) const override;
};


int32_t close_safe(int32_t fd);

int32_t open_safe(const char *pathname, int32_t flags);

int32_t open_safe(const char *pathname, int32_t flags, mode_t mode);

ssize_t read_safe(int32_t fd, void *buf, size_t len, const string &file_name);

ssize_t write_safe(int32_t fd, const void *buf, size_t len, const string &file_name);


string f$basename(const string &name, const string &suffix = string());

bool f$chmod(const string &s, int64_t mode);

void f$clearstatcache();

bool f$copy(const string &from, const string &to);

string f$dirname(const string &name);

Optional<array<string>> f$file(const string &name);

bool f$file_exists(const string &name);

Optional<int64_t> f$filesize(const string &name);

Optional<int64_t> f$filectime(const string &name);

Optional<int64_t> f$filemtime(const string &name);

bool f$is_dir(const string &name);

bool f$is_file(const string &name);

bool f$is_readable(const string &name);

bool f$is_writeable(const string &name);

bool f$mkdir(const string &name, int64_t mode = 0777, bool recursive = false);

string f$php_uname(const string &name = LETTER_a);

bool f$rename(const string &oldname, const string &newname);

Optional<string> f$realpath(const string &path);

Optional<string> f$tempnam(const string &dir, const string &prefix);

bool f$unlink(const string &name);

Optional<array<string>> f$scandir(const string &directory);

Optional<string> file_file_get_contents(const string &name);


void global_init_files_lib();

void free_files_lib();
