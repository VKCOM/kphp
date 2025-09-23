// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

namespace kphp_ml {
struct MLModel;
} // namespace kphp_ml

void kml_init_models();
void kml_init_buffer();

std::byte* kphp_ml_get_mutable_buffer_in_current_worker();
const kphp_ml::MLModel* kphp_ml_find_loaded_model_by_name(const string& model_name);
