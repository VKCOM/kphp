// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <xgboost/learner.h>
#include <xgboost/simple_dmatrix.h>

#include <utility>

#include "runtime/xgboost/model.h"
#include "runtime/xgboost/array-adapter.h"

namespace vk {
namespace xgboost {

void Model::init(vk::string_view path) {
  std::unique_ptr<::xgboost::Learner> booster{::xgboost::Learner::Create({})};
  std::unique_ptr<dmlc::Stream> stream{dmlc::Stream::Create(path.data(), "r")};
  booster->LoadModel(stream.get());
  booster->Configure();
  booster_ = std::move(booster);
}

void Model::reset() {
  booster_.reset();
}

void Model::predict(const array<array<double>> &features, ::xgboost::PredictionCacheEntry &scores) const {
  if (!booster_) {
    throw std::runtime_error{"there is no xgboost model loaded"};
  }
  scores.predictions.HostVector().clear();
  scores.version = 0;
  scores.ref.reset();

  assert(features.size().string_size == 0);
  ArrayAdapter adapter{features, static_cast<std::size_t>(features.size().int_size), booster_->GetNumFeature()};
  ::xgboost::data::SimpleDMatrix features_matrix{&adapter, std::nanf(""), 1};
  booster_->PredictDefault(features_matrix, false, scores, 0, 0);
}

} // namespace xgboost
} // namespace vk

void global_init_xgboost_lib() {
  const auto &path = vk::singleton<vk::xgboost::ModelPath>::get().path;
  if (!path.empty()) {
    vk::singleton<vk::xgboost::Model>::get().init(path);
  }
}

void shutdown_xgboost_lib() {
  vk::singleton<vk::xgboost::Model>::get().reset();
}
