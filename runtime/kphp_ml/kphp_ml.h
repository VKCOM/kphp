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

#include <optional>
#include <string>
#include <variant>

#include "runtime/kphp_ml/kphp_ml_catboost.h"
#include "runtime/kphp_ml/kphp_ml_xgboost.h"

namespace kphp_ml {

constexpr int KML_FILE_PREFIX = 0x718249F0;
constexpr int KML_FILE_VERSION_100 = 100;

enum class ModelKind {
  invalid_kind,
  xgboost_trees_no_cat,
  catboost_trees,
};

enum class InputKind {
  // [ 'user_city_99' => 1, 'user_topic_weights_17' => 7.42, ...], uses reindex_map
  ht_remap_str_keys_to_fvalue,
  // [ 12934 => 1, 8923 => 7.42, ... ], uses reindex_map
  ht_remap_int_keys_to_fvalue,
  // [ 70 => 1, 23 => 7.42, ... ], no keys reindex, pass directly
  ht_direct_int_keys_to_fvalue,

  // [ 1.23, 4.56, ... ] and [ "red", "small" ]: floats and cat separately, pass directly
  vectors_fvalue_and_catstr,
  // the same, but a model is a catboost multiclassificator (returns an array of predictions, not one)
  vectors_fvalue_and_catstr_multi,
  // [ 'emb_7' => 19.98, ..., 'user_os' => 2, ... ]: in one ht, but categorials are numbers also (to avoid `mixed`)
  ht_remap_str_keys_to_fvalue_or_catnum,
  // the same, but multiclassificator (returns an array of predictions, not one)
  ht_remap_str_keys_to_fvalue_or_catnum_multi,
};

struct MLModel {
  ModelKind model_kind;
  InputKind input_kind;
  std::string model_name;
  std::vector<std::string> feature_names;
  std::unordered_map<std::string, std::string> custom_properties;

  std::variant<kphp_ml_xgboost::XgboostModel, kphp_ml_catboost::CatboostModel> impl;

  bool is_xgboost() const {
    return model_kind == ModelKind::xgboost_trees_no_cat;
  }
  bool is_catboost() const {
    return model_kind == ModelKind::catboost_trees;
  }
  bool is_catboost_multi_classification() const;
  const std::vector<std::string>& get_feature_names() const;
  std::optional<std::string> get_custom_property(const std::string& property_name) const;

  unsigned int calculate_mutable_buffer_size() const;
};

} // namespace kphp_ml
