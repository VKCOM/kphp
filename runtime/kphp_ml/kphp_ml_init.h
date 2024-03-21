#pragma once

#include "runtime/kphp_core.h"

namespace kphp_ml { struct MLModel; }

extern const char *KmlDirectory;

void init_kphp_ml_runtime_in_master();
void init_kphp_ml_runtime_in_worker();


char *kphp_ml_get_mutable_buffer_in_current_worker();
const kphp_ml::MLModel *kphp_ml_find_loaded_model_by_name(const string &model_name);
