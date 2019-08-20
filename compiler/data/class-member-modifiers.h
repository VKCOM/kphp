#pragma once

#include <cstdint>

enum class ScopeModifiers : uint8_t {
  not_member_,
  static_,
  instance_,
};

enum class AbstractModifiers : uint8_t {
  not_modifier_,
  final_,
  abstract_,
};

enum class AccessModifiers : uint8_t {
  not_modifier_,
  private_,
  public_,
  protected_,
};
