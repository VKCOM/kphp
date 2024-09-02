// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/class-instance/refcountable-php-classes.h"

struct C$ComponentStream final : public refcountable_php_classes<C$ComponentStream> {
  uint64_t stream_d{};

  const char *get_class() const noexcept;

  int32_t get_hash() const noexcept;

  ~C$ComponentStream();
};

struct C$ComponentQuery final : public refcountable_php_classes<C$ComponentQuery> {
  uint64_t stream_d{};

  const char *get_class() const noexcept;

  int32_t get_hash() const noexcept;

  ~C$ComponentQuery();
};

bool f$ComponentStream$$is_read_closed(const class_instance<C$ComponentStream> &stream);
bool f$ComponentStream$$is_write_closed(const class_instance<C$ComponentStream> &stream);
bool f$ComponentStream$$is_please_shutdown_write(const class_instance<C$ComponentStream> &stream);

void f$ComponentStream$$close(const class_instance<C$ComponentStream> &stream);
void f$ComponentStream$$shutdown_write(const class_instance<C$ComponentStream> &stream);
void f$ComponentStream$$please_shutdown_write(const class_instance<C$ComponentStream> &stream);
