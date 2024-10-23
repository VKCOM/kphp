// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/runtime-core/runtime-core.h"

namespace kphp_ml { struct MLModel; }

extern const char *kml_directory;

void init_kphp_ml_runtime_in_master();
void init_kphp_ml_runtime_in_worker();


char *kphp_ml_get_mutable_buffer_in_current_worker();
const kphp_ml::MLModel *kphp_ml_find_loaded_model_by_name(const string &model_name);
