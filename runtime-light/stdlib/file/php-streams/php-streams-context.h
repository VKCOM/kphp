// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/stdlib/file/php-streams/php-streams.h"

struct PhpStreamInstanceState final : private vk::not_copyable {
  class_instance<resource_impl_::PhpResourceWrapper> stdin_wrapper;
  class_instance<resource_impl_::PhpResourceWrapper> stdout_wrapper;
  class_instance<resource_impl_::PhpResourceWrapper> stderr_wrapper;

  static PhpStreamInstanceState &get() noexcept;
};
