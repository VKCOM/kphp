// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime/pdo/abstract_pdo_statement.h"

struct C$PDOStatement : public refcountable_polymorphic_php_classes<abstract_refcountable_php_interface>, private DummyVisitorMethods {
  std::unique_ptr<pdo::AbstractPdoStatement> statement;
  int64_t timeout_sec{-1};

  ~C$PDOStatement() override = default;

  const char *get_class() const noexcept {
    return "PDOStatement";
  }

  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$PDOStatement::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  using DummyVisitorMethods::accept;
};

bool f$PDOStatement$$execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params = {}) noexcept;

mixed f$PDOStatement$$fetch(const class_instance<C$PDOStatement> &v$this) noexcept;
array<mixed> f$PDOStatement$$fetchAll(const class_instance<C$PDOStatement> &v$this) noexcept;
