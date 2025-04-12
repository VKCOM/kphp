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
#include "runtime/pdo/abstract_pdo_driver.h"

struct C$PDO : public refcountable_polymorphic_php_classes_virt<>, private DummyVisitorMethods {
  static constexpr int ATTR_TIMEOUT = 2;

  std::unique_ptr<pdo::AbstractPdoDriver> driver;
  int64_t timeout_sec{-1};

  ~C$PDO() override = default;

  virtual const char* get_class() const noexcept {
    return "PDO";
  }

  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$PDO::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  using DummyVisitorMethods::accept;
};

class_instance<C$PDO> f$PDO$$__construct(class_instance<C$PDO> const& v$this, const string& dsn, const Optional<string>& username = {},
                                         const Optional<string>& password = {}, const Optional<array<mixed>>& options = {}) noexcept;

struct C$PDOStatement;

class_instance<C$PDOStatement> f$PDO$$prepare(const class_instance<C$PDO>& v$this, const string& query, const array<mixed>& options = {}) noexcept;

class_instance<C$PDOStatement> f$PDO$$query(const class_instance<C$PDO>& v$this, const string& query, Optional<int64_t> fetchMode = {});

Optional<int64_t> f$PDO$$exec(const class_instance<C$PDO>& v$this, const string& query);

Optional<string> f$PDO$$errorCode(const class_instance<C$PDO>& v$this);
array<mixed> f$PDO$$errorInfo(const class_instance<C$PDO>& v$this);
