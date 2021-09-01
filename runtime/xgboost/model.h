// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/wrappers/string_view.h"
#include "runtime/kphp_core.h"

namespace xgboost {
class Learner;
class PredictionCacheEntry;
} // namespace xgboost

namespace vk {
namespace xgboost {

struct ModelPath : vk::not_copyable {
  std::string path;

private:
  friend class vk::singleton<ModelPath>;
  ModelPath() = default;
};

class Model : vk::not_copyable {
public:
  void init(vk::string_view path);
  void reset();
  void predict(const array<array<double>> &features, ::xgboost::PredictionCacheEntry &scores) const;

private:
  friend class vk::singleton<Model>;
  Model() = default;

  std::unique_ptr<const ::xgboost::Learner> booster_;
};

} // namespace xgboost
} // namespace vk

void global_init_xgboost_lib();
void shutdown_xgboost_lib();
