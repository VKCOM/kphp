// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

enum class ScopeModifiers : uint8_t {
  not_member_,
  static_,
  instance_,
  static_lambda_,
};

enum class AbstractModifiers : uint8_t {
  not_modifier_,
  final_,
  abstract_,
  readonly_,
};

enum class AccessModifiers : uint8_t {
  not_modifier_,
  private_,
  public_,
  protected_,
};
