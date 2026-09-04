// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "compiler/function-pass.h"

class CollectForkableTypesPass final : public FunctionPassBase {
public:
  std::string get_description() final {
    return "Collect forkable types";
  }

  VertexPtr on_enter_vertex(VertexPtr root) final;

  void on_finish() final;

private:
  std::vector<const TypeData*> waitable_types_;
  std::vector<const TypeData*> forkable_types_;
};

class ForkableTypeStorage : vk::not_copyable {
public:
  friend class vk::singleton<ForkableTypeStorage>;

  void add_types(const std::vector<const TypeData*>& waitable_types, const std::vector<const TypeData*>& forkable_types) noexcept;

  std::vector<const TypeData*> flush_waitable_types() noexcept;
  std::vector<const TypeData*> flush_forkable_types() noexcept;

private:
  ForkableTypeStorage() = default;

  std::mutex mutex_;
  std::vector<const TypeData*> waitable_types_;
  std::vector<const TypeData*> forkable_types_;
};
