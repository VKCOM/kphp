// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

/*
# About .kml files and kphp_ml in general

KML means "KPHP ML", since it was invented for KPHP and VK.com.
KML unites xgboost and catboost (prediction only, not learning).
KML models are stored in files with *.kml* extension.

KML is several times faster compared to native xgboost
and almost identical compared to native catboost.

A final structure integrated into KPHP consists of the following:
1) custom xgboost implementation
2) custom catboost implementation
3) .kml files reader
4) buffers and kml models storage related to master-worker specifics
5) api to be called from PHP code

To use ML from PHP code, call any function from `kphp_ml_interface.h` (KPHP only).
In plain PHP, there are no polyfills, and they are not planned to be implemented.

# About "ml_experiments" private vkcom repo

The code in the `kphp_ml` namespace is a final, production solution.

While development, we tested lots of various implementations (both for xgboost/catboost)
in order to find an optimal one — they are located in the `ml_experiments` repository.

All in all, `ml_experiments` repo contains:
1) lots of C++ implementations of algorithms that behave exactly like xgboost/catboost
2) tooling for testing and benchmarking them
3) ML models to be tested and benchmarked (some of them are from real production)
4) python scripts for learning and converting models
5) a converter to .kml

Note, that some files exist both in KPHP and `ml_experiments`.
They are almost identical, besides include paths and input types (`array` vs `unordered_map`).
In the future development, they should be maintained synchronized.

# Application-specific information in kml

When a learned model is exported to xgboost .model file or catboost .cbm file,
it **does not** contain enough information to be evaluated.
Some information exists only at the moment of learning and thus must also be saved
along with xgboost/catboost exported models.

For example, a prediction might need calibration (`*MULT+BIAS` or `log`) **AFTER** xgboost calculation.

For example, input `[1234 => 0.98]` (feature_id #1234) must be remapped before passing to xgboost,
because this feature was #42 while training, but a valid input is #1234.
Hence, `[1234 => 42]` exists in reindex map.

For example, some models were trained without zero values, and zeroes in input must be excluded.

Ideally, an input should always contain correct indexes and shouldn't contain zeroes it the last case,
but in practice in VK.com, inputs are collected universally, and later applied to some model.
That's why one and the same input is remapped by model1 in a way 1, and by model2 in its own way.

As a conclusion, training scripts must export not only xgboost/catboost models, but a .json file
with additional properties also — for converting to .kml and evaluating.
See `KmlPropertiesInJsonFile` in `ml_experiments`.

.kml files, on the contrary, already contain all additional information inside,
because exporting to kml requires all that stuff.

# InputKind

Ideally, backend code must collect input that should be passed to a model directly.
For example, if a model was trained with features #1...#100,
an input could look like `[ 70 => 1.0, 23 => 7.42, ... ]`.

But in practice and due to historical reasons, vkcom backend collects input in a different way,
and it can't be passed directly. It needs some transformations. Available types of input
and its transformation is `enum InputKind`, see below.

# KML inference speed compared to xgboost/catboost

Benchmarking shows, that a final KML predictor works 3–10 times faster compared to native xgboost.

This is explained by several reasons and optimizations:
* compressed size of a tree node (8 bytes only)
* coordinates remapping
* better cache locality
* input vectorization and avoiding `if`s in code

Remember, that KPHP workers are single-threaded, that's why it's compared with xgboost working
on a single thread, no GPU.

.kml files are much more lightweight than .model xgboost files, since nodes are compressed
and all learning info is omitted. They can be loaded into memory very quickly, almost as POD bytes reading.

When it comes to catboost, KML implementation is almost identical to
[native](https://github.com/catboost/catboost/blob/master/catboost/libs/model/model_export/resources).
But .kml files containing catboost models are also smaller than original .cbm files.

# KPHP-specific implementation restrictions

After PHP code is compiled to a server binary, it's launched as a pre-fork server.

The master process loads all .kml files from the folder (provided as a cmd line option).
Note, that storage of models (and data of every model itself) is read-only,
that's why it's not copied to every process, and we are allowed to use `std` containers there.

After fork, when PHP script is executed by every worker, it executes prediction, providing an input (PHP `array`).

KPHP internals should be very careful of using std containers inside workers, since they allocate in heap,
which generally is bad because of signals handling. That's why KML evaluation doesn't use heap at all,
but when it needs memory for performing calculations, it uses pre-allocated `mutable_buffer`.
That mutable buffer is allocated once at every worker process start up,
its size is `max(calculate_mutable_buffer_size(i))`. Hence, it can fit any model calculation.

A disappointing fact is that KPHP `array` is quite slow compared to `std::unordered_map`,
that's why a native C++ implementation is faster than a KPHP one
when an algorithm needs to iterate over input hashtables.

# Looking backward: a brief history of ML in VK.com

Historically, ML infrastructure in production was quite weird: ML models were tons of .php files
with autogenerated PHP code of decision trees, like
```php
function some_long_model_name_xxx_score_tree_3(array $x) {
  if ($x[1657] < 0.00926230289) {
    if ($x[1703] < 0.00839830097) {
      if ($x[1657] < 0.00389328809) {
        if ($x[1656] < 0.00147941126) {
          return -0.216939136;}
        return -0.215985224;}
  ...
}
```

Hundreds of .php files, with hundreds of functions within each, with lots of lines `if else if else`
accessing input hashtables, sometimes transformed into vectors.

That autogenerated code was placed in a separate repository, compiled with KPHP `-M lib`, and linked
into `vkcom` binary upon final compilation. The amount of models was so huge, that they took about 600 MB
of 1.5 GB production binary. The speed of inference, nevertheless, was quite fast,
especially when hashtables were transformed to vectors in advance.

Time passed, and we decided to rewrite ML infrastructure from scratch. The goal was to
1) Get rid of codegenerated PHP code at all.
2) Greatly speed up current production.
3) Support catboost and categorial features.

Obviously, there were two possible directions:
1) Import native xgboost and catboost libraries into KPHP runtime and write some transformers
   from PHP input to native calls; store .model and .cbm files which can be loaded and executed.
2) Write a custom ML prediction kernel that works exactly like native xgboost/catboost,
   but (if possible) much faster and much more lightweight; implement some .kml file format storing ML models.

As one may guess, we finally head the second way.

# Looking forward: possible future enhancements

For now, provided solution it more than enough and solves all problems we face nowadays.
In the future, the following points might be considered as areas of investigation.

* Support embedded and text features in catboost.
* Support onnx kernel for neural networks (also a custom implementation, of course).
* Use something more effective than `std::unordered_map` for reindex maps.
* Implement a thread pool in KPHP and parallelize inputs; it's safe, since they are read only.
*/

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

#include "runtime-common/core/std/containers.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/kml/catboost.h"
#include "runtime-common/stdlib/kml/file-api.h"
#include "runtime-common/stdlib/kml/input_kind.h"
#include "runtime-common/stdlib/kml/xgboost.h"

namespace kphp::kml {

inline constexpr int32_t KML_FILE_PREFIX = 0x718249F0;
inline constexpr int32_t KML_FILE_VERSION_100 = 100;

namespace detail {

enum class model_kind {
  invalid_kind,
  xgboost_trees_no_cat,
  catboost_trees,
};

} // namespace detail

template<template<class> class Allocator>
class model {
  kphp::kml::input_kind m_input_kind;
  kphp::stl::string<Allocator> m_name;
  kphp::stl::vector<kphp::stl::string<Allocator>, Allocator> m_feature_names;
  kphp::stl::unordered_map<kphp::stl::string<Allocator>, kphp::stl::string<Allocator>, Allocator> m_custom_properties;

  std::variant<std::monostate, kphp::kml::xgboost::model<Allocator>, kphp::kml::catboost::model<Allocator>> m_impl;

public:
  std::string_view name() const noexcept {
    return m_name;
  }

  kphp::kml::input_kind input_kind() const noexcept {
    return m_input_kind;
  }

  const auto& feature_names() const noexcept {
    return m_feature_names;
  }

  std::optional<std::string_view> custom_property(const kphp::stl::string<Allocator>& property_name) const noexcept {
    if (auto it{m_custom_properties.find(property_name)}; it != m_custom_properties.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  size_t mutable_buffer_size() const noexcept {
    if (auto xgb{as_xgboost()}; xgb) {
      return (*xgb).get().mutable_buffer_size();
    } else if (auto cbm{as_catboost()}; cbm) {
      return (*cbm).get().mutable_buffer_size();
    }
    return 0;
  }

  std::optional<std::reference_wrapper<const kphp::kml::xgboost::model<Allocator>>> as_xgboost() const noexcept {
    if (auto* xgb{std::get_if<kphp::kml::xgboost::model<Allocator>>(std::addressof(m_impl))}; xgb != nullptr) {
      return *xgb;
    }
    return std::nullopt;
  }

  std::optional<std::reference_wrapper<const kphp::kml::catboost::model<Allocator>>> as_catboost() const noexcept {
    if (auto* cbm{std::get_if<kphp::kml::catboost::model<Allocator>>(std::addressof(m_impl))}; cbm != nullptr) {
      return *cbm;
    }
    return std::nullopt;
  }

  template<class FileReader>
  static std::optional<model<Allocator>> load(FileReader file_reader) noexcept {
    detail::reader<FileReader, Allocator> kml_reader{std::move(file_reader)};

    if (kml_reader.read_int32() != KML_FILE_PREFIX) {
      php_warning("failed to load KML model: bad .kml file prefix");
      return std::nullopt;
    }

    auto version = kml_reader.read_int32();
    if (version != KML_FILE_VERSION_100) {
      php_warning("failed to load KML model: bad version");
      return std::nullopt;
    }

    model<Allocator> kml{};

    detail::model_kind model_kind{};
    kml_reader.read_enum(model_kind);

    kml_reader.read_enum(kml.m_input_kind);
    kml_reader.read_string(kml.m_name);
    kml_reader.read_vec(kml.m_feature_names);

    auto custom_props_sz = kml_reader.read_int32();
    kml.m_custom_properties.reserve(custom_props_sz);
    for (auto i = 0; i < custom_props_sz; ++i) {
      kphp::stl::string<Allocator> property_name;
      kml_reader.read_string(property_name);
      kml_reader.read_string(kml.m_custom_properties[property_name]);
    }

    if (kml_reader.is_eof()) {
      php_warning("failed to load KML model: unexpected EOF");
      return std::nullopt;
    }

    switch (model_kind) {
    case detail::model_kind::xgboost_trees_no_cat: {
      auto opt_xgb = kphp::kml::xgboost::model<Allocator>::load(kml_reader);
      if (!opt_xgb) {
        return std::nullopt;
      }
      kml.m_impl = *std::move(opt_xgb);
      break;
    }
    case detail::model_kind::catboost_trees: {
      auto opt_catboost = kphp::kml::catboost::model<Allocator>::load(kml_reader);
      if (!opt_catboost) {
        return std::nullopt;
      }
      kml.m_impl = *std::move(opt_catboost);
      break;
    }
    default:
      php_warning("failed to load KML model: unsupported model kind");
      return std::nullopt;
    }

    return kml;
  }
};

} // namespace kphp::kml
