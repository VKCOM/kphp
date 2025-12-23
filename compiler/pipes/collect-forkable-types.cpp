// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/collect-forkable-types.h"

#include "common/algorithms/sorting.h"
#include "compiler/inferring/public.h"

VertexPtr CollectForkableTypesPass::on_enter_vertex(VertexPtr root) {
  if (auto call = root.try_as<op_func_call>()) {
    if (call->func_id->is_resumable || call->func_id->is_k2_fork || call->func_id->is_interruptible) {
      if (call->str_val == "wait") {
        waitable_types_.push_back(tinf::get_type(root));
      } else if (call->str_val == "wait_multi") {
        forkable_types_.push_back(tinf::get_type(root)->lookup_at_any_key());
        waitable_types_.push_back(tinf::get_type(root)->lookup_at_any_key());
      }
      forkable_types_.push_back(tinf::get_type(root));
    } else if (call->str_val == "wait_synchronously") {
      waitable_types_.push_back(tinf::get_type(root));
    }
  }
  return root;
}

void CollectForkableTypesPass::on_finish() {
  vk::sort_and_unique(waitable_types_);
  vk::sort_and_unique(forkable_types_);
  if (!waitable_types_.empty() || !forkable_types_.empty()) {
    vk::singleton<ForkableTypeStorage>::get().add_types(waitable_types_, forkable_types_);
  }
}

void ForkableTypeStorage::add_types(const std::vector<const TypeData *> &waitable_types, const std::vector<const TypeData *> &forkable_types) noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  waitable_types_.insert(waitable_types_.end(), waitable_types.begin(), waitable_types.end());
  forkable_types_.insert(forkable_types_.end(), forkable_types.begin(), forkable_types.end());
}

std::vector<const TypeData *> ForkableTypeStorage::flush_waitable_types() noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  return std::move(waitable_types_);
}

std::vector<const TypeData *> ForkableTypeStorage::flush_forkable_types() noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  return std::move(forkable_types_);
}
