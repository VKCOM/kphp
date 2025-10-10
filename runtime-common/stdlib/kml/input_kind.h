// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

namespace kphp::kml {

enum class input_kind {
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

} // namespace kphp::kml
