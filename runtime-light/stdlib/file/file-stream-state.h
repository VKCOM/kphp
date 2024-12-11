// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/file/file-resource.h"
#include "runtime-light/stdlib/file/php-streams/php-streams.h"

struct FileStreamInstanceState final : private vk::not_copyable {
  mixed error_number_dummy;
  mixed error_description_dummy;

  class_instance<resource_impl_::PhpResourceWrapper> stdin_wrapper;
  class_instance<resource_impl_::PhpResourceWrapper> stdout_wrapper;
  class_instance<resource_impl_::PhpResourceWrapper> stderr_wrapper;

  static FileStreamInstanceState &get() noexcept;
};
