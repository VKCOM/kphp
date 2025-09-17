// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#include "runtime-common/stdlib/kml/kml-files-reader.h"
#include "common/mixin/not_copyable.h"
#include "common/wrappers/overloaded.h"
#include "runtime-common/core/allocator/platform-allocator.h"

#include <cstdio>
#include <variant>

/*
 * .kml files unite xgboost and catboost (prediction only, not learning).
 * This module reads .kml files into kphp_ml::MLModel.
 *
 * In KPHP, .kml files are read at master process start up
 * and used in read-only mode by all worker processes when a backend calls ML inference.
 *
 * There is also kml-files-writer.cpp, very similar to a reader.
 * But it's not a part of KPHP, it's a part of "ml_experiments" repo.
 */

namespace {

// Not really efficient, but it's OK as reading ML models once at start
template<typename T>
using Result = std::variant<T, std::string>;

template<typename T>
[[nodiscard]] std::optional<T> get_value(const Result<T>& result) {
  return std::visit(overloaded{[](const T& value) -> std::optional<T> { return value; }, [](const std::string&) -> std::optional<T> { return std::nullopt; }},
                    result);
}

template<typename T>
[[nodiscard]] std::optional<std::string> get_error(const Result<T>& result) {
  return std::visit(overloaded{[](const T&) -> std::optional<std::string> { return std::nullopt; },
                               [](const std::string& error) -> std::optional<std::string> { return error; }},
                    result);
}

class KmlReader final : public vk::not_copyable {
  FileReader reader;

public:
  explicit KmlReader(vk::string_view kml_filename)
      : reader{kml_filename} {}

  int read_int() noexcept {
    int v;
    reader.read((char*)&v, sizeof(int));
    return v;
  }

  void read_int(int& v) noexcept {
    reader.read((char*)&v, sizeof(int));
  }

  void read_uint32(uint32_t& v) noexcept {
    reader.read((char*)&v, sizeof(uint32_t));
  }

  void read_uint64(uint64_t& v) noexcept {
    reader.read((char*)&v, sizeof(uint64_t));
  }

  void read_float(float& v) noexcept {
    reader.read((char*)&v, sizeof(float));
  }

  void read_double(double& v) noexcept {
    reader.read((char*)&v, sizeof(double));
  }

  template<class T>
  void read_enum(T& v) noexcept {
    static_assert(sizeof(T) == sizeof(int));
    reader.read((char*)&v, sizeof(int));
  }

  void read_string(std::string& v) noexcept {
    int len;
    reader.read((char*)&len, sizeof(int));
    v.resize(len);
    reader.read(v.data(), len);
  }

  void read_bool(bool& v) noexcept {
    v = static_cast<bool>(read_int());
  }

  void read_bytes(void* v, size_t len) noexcept {
    reader.read((char*)v, static_cast<std::streamsize>(len));
  }

  template<class T>
  void read_vec(std::vector<T>& v) {
    static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>);
    int sz;
    read_int(sz);
    v.resize(sz);
    read_bytes(v.data(), sz * sizeof(T));
  }

  template<class T>
  void read_2d_vec(std::vector<std::vector<T>>& v) {
    int sz;
    read_int(sz);
    v.resize(sz);
    for (std::vector<T>& elem : v) {
      read_vec(elem);
    }
  }

  [[nodiscard]] Result<std::monostate> check_not_eof() const {
    if (reader.is_eof()) {
      return "unexpected eof";
    }
    return std::monostate{};
  }
};

template<>
void KmlReader::read_vec<std::string>(std::vector<std::string>& v) {
  int sz = read_int();
  v.resize(sz);
  for (auto& str : v) {
    read_string(str);
  }
}

void kml_read_catboost_field(KmlReader& f, kphp_ml_catboost::CatboostProjection& v) {
  f.read_vec(v.transposed_cat_feature_indexes);
  f.read_vec(v.binarized_indexes);
}

void kml_read_catboost_field(KmlReader& f, kphp_ml_catboost::CatboostModelCtr& v) {
  f.read_uint64(v.base_hash);
  f.read_enum(v.base_ctr_type);
  f.read_int(v.target_border_idx);
  f.read_float(v.prior_num);
  f.read_float(v.prior_denom);
  f.read_float(v.shift);
  f.read_float(v.scale);
}

void kml_read_catboost_field(KmlReader& f, kphp_ml_catboost::CatboostCompressedModelCtr& v) {
  kml_read_catboost_field(f, v.projection);

  int sz = f.read_int();
  v.model_ctrs.resize(sz);
  for (auto& item : v.model_ctrs) {
    kml_read_catboost_field(f, item);
  }
}

void kml_read_catboost_field(KmlReader& f, kphp_ml_catboost::CatboostCtrValueTable& v) {
  int sz = f.read_int();
  v.index_hash_viewer.reserve(sz);
  for (int i = 0; i < sz; ++i) {
    uint64_t k;
    f.read_uint64(k);
    f.read_uint32(v.index_hash_viewer[k]);
  }

  f.read_int(v.target_classes_count);
  f.read_int(v.counter_denominator);
  f.read_vec(v.ctr_mean_history);
  f.read_vec(v.ctr_total);
}

void kml_write_catboost_field(KmlReader& f, kphp_ml_catboost::CatboostCtrData& v) {
  int sz = f.read_int();
  v.learn_ctrs.reserve(sz);
  for (int i = 0; i < sz; ++i) {
    uint64_t key;
    f.read_uint64(key);
    kml_read_catboost_field(f, v.learn_ctrs[key]);
  }
}

void kml_read_catboost_field(KmlReader& f, kphp_ml_catboost::CatboostModelCtrsContainer& v) {
  bool has;
  f.read_bool(has);

  if (has) {
    f.read_int(v.used_model_ctrs_count);

    int sz = f.read_int();
    v.compressed_model_ctrs.resize(sz);
    for (auto& item : v.compressed_model_ctrs) {
      kml_read_catboost_field(f, item);
    }

    kml_write_catboost_field(f, v.ctr_data);
  }
}

// -------------

[[nodiscard]] Result<std::monostate> kml_file_read_xgboost_trees_no_cat(KmlReader& f, [[maybe_unused]] int version, kphp_ml_xgboost::XgboostModel& xgb) {
  f.read_enum(xgb.tparam_objective);
  f.read_bytes(&xgb.calibration, sizeof(kphp_ml_xgboost::CalibrationMethod));
  f.read_float(xgb.base_score);
  f.read_int(xgb.num_features_trained);
  f.read_int(xgb.num_features_present);
  f.read_int(xgb.max_required_features);

  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  if (xgb.num_features_present <= 0 || xgb.num_features_present > xgb.max_required_features) {
    return "wrong num_features_present";
  }

  int num_trees = f.read_int();
  if (num_trees <= 0 || num_trees > 10000) {
    return "wrong num_trees";
  }

  xgb.trees.resize(num_trees);
  for (kphp_ml_xgboost::XgbTree& tree : xgb.trees) {
    int num_nodes = f.read_int();
    if (num_nodes <= 0 || num_nodes > 10000) {
      return "wrong num_nodes";
    }
    tree.nodes.resize(num_nodes);
    f.read_bytes(tree.nodes.data(), sizeof(kphp_ml_xgboost::XgbTreeNode) * num_nodes);
  }

  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  xgb.offset_in_vec = kphp::memory::platform_allocator<int>{}.allocate(xgb.max_required_features);
  f.read_bytes(xgb.offset_in_vec, xgb.max_required_features * sizeof(int));
  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  xgb.reindex_map_int2int = kphp::memory::platform_allocator<int>{}.allocate(xgb.max_required_features);
  f.read_bytes(xgb.reindex_map_int2int, xgb.max_required_features * sizeof(int));
  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  int num_reindex_str2int = f.read_int();
  if (num_reindex_str2int < 0 || num_reindex_str2int > xgb.max_required_features) {
    return "wrong num_reindex_str2int";
  }
  for (int i = 0; i < num_reindex_str2int; ++i) {
    uint64_t hash;
    f.read_uint64(hash);
    f.read_int(xgb.reindex_map_str2int[hash]);
  }
  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  f.read_bool(xgb.skip_zeroes);
  f.read_float(xgb.default_missing_value);

  return std::monostate{};
}

[[nodiscard]] Result<std::monostate> kml_file_read_catboost_trees(KmlReader& f, [[maybe_unused]] int version, kphp_ml_catboost::CatboostModel& cbm) {
  f.read_int(cbm.float_feature_count);
  f.read_int(cbm.cat_feature_count);
  f.read_int(cbm.binary_feature_count);
  f.read_int(cbm.tree_count);
  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  f.read_vec(cbm.float_features_index);
  f.read_2d_vec(cbm.float_feature_borders);
  f.read_vec(cbm.tree_depth);
  f.read_vec(cbm.one_hot_cat_feature_index);
  f.read_2d_vec(cbm.one_hot_hash_values);
  f.read_2d_vec(cbm.ctr_feature_borders);
  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  f.read_vec(cbm.tree_split);
  f.read_vec(cbm.leaf_values);
  f.read_2d_vec(cbm.leaf_values_vec);
  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  f.read_double(cbm.scale);
  f.read_double(cbm.bias);
  f.read_vec(cbm.biases);
  f.read_int(cbm.dimension);
  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  int cat_hashes_sz = f.read_int();
  cbm.cat_features_hashes.reserve(cat_hashes_sz);
  for (int i = 0; i < cat_hashes_sz; ++i) {
    uint64_t key_hash;
    f.read_uint64(key_hash);
    f.read_int(cbm.cat_features_hashes[key_hash]);
  }

  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  kml_read_catboost_field(f, cbm.model_ctrs);

  int reindex_sz = f.read_int();
  cbm.reindex_map_floats_and_cat.reserve(reindex_sz);
  for (int i = 0; i < reindex_sz; ++i) {
    uint64_t hash;
    f.read_uint64(hash);
    f.read_int(cbm.reindex_map_floats_and_cat[hash]);
  }

  return std::monostate{};
}

} // namespace

[[nodiscard]] std::variant<kphp_ml::MLModel, std::string> kml_file_read(const std::string& kml_filename) {
  kphp_ml::MLModel kml;
  KmlReader f(kml_filename);

  if (f.read_int() != kphp_ml::KML_FILE_PREFIX) {
    return "wrong .kml file prefix";
  }
  int version = f.read_int();
  if (version != kphp_ml::KML_FILE_VERSION_100) {
    return "wrong .kml file version";
  }

  f.read_enum(kml.model_kind);
  f.read_enum(kml.input_kind);
  f.read_string(kml.model_name);
  f.read_vec(kml.feature_names);

  int custom_props_sz = f.read_int();
  kml.custom_properties.reserve(custom_props_sz);
  for (int i = 0; i < custom_props_sz; ++i) {
    std::string property_name;
    f.read_string(property_name);
    f.read_string(kml.custom_properties[property_name]);
  }

  if (auto err = get_error(f.check_not_eof()); err.has_value()) {
    return *err;
  }

  switch (kml.model_kind) {
  case kphp_ml::ModelKind::xgboost_trees_no_cat: {
    kml.impl = kphp_ml_xgboost::XgboostModel();
    auto res = kml_file_read_xgboost_trees_no_cat(f, version, std::get<kphp_ml_xgboost::XgboostModel>(kml.impl));
    if (auto err = get_error(res); err.has_value()) {
      return *err;
    }
    break;
  }
  case kphp_ml::ModelKind::catboost_trees: {
    kml.impl = kphp_ml_catboost::CatboostModel();
    auto res = kml_file_read_catboost_trees(f, version, std::get<kphp_ml_catboost::CatboostModel>(kml.impl));
    if (auto err = get_error(res); err.has_value()) {
      return *err;
    }
    break;
  }
  default:
    return "unsupported model_kind";
  }

  return kml;
}
