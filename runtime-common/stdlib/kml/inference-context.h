// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/std/containers.h"

template<template<class> class Allocator>
struct KmlInferenceContext final : vk::not_copyable {
  kphp::stl::vector<std::byte, Allocator> m_buffer;

  void init(size_t buffer_size) noexcept {
    m_buffer.resize(buffer_size);
  }

  KmlInferenceContext() noexcept = default;

  static KmlInferenceContext& get() noexcept;
};
