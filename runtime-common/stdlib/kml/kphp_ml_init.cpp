// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/kml/kphp_ml_init.h"

#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <variant>

#include "runtime-common/core/allocator/platform-allocator.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/kml/kml-files-reader.h"
#include "runtime-common/stdlib/kml/kml-inference-context.h"
#include "runtime-common/stdlib/kml/kml-models-context.h"
#include "runtime-common/stdlib/kml/kphp_ml.h"

static bool ends_with(const char* str, const char* suffix) {
  size_t len_str = strlen(str);
  size_t len_suffix = strlen(suffix);

  return len_suffix <= len_str && strncmp(str + len_str - len_suffix, suffix, len_suffix) == 0;
}

static void load_kml_file(const std::string& path) {
  kphp_ml::MLModel kml;

  auto res = kml_file_read(path);
  if (std::holds_alternative<std::string>(res)) {
    php_warning("cannot read %s: %s\n", path.c_str(), std::get<std::string>(res).c_str());
    return;
  }
  kml = std::get<kphp_ml::MLModel>(res);

  auto& kml_models_context = KmlModelsContext::get_mutable();

  unsigned int buffer_size = kml.calculate_mutable_buffer_size();
  kml_models_context.max_mutable_buffer_size = std::max(kml_models_context.max_mutable_buffer_size, (buffer_size + 3) & -4);

  uint64_t key_hash = string_hash(kml.model_name.c_str(), kml.model_name.size());
  if (auto dup_it = kml_models_context.loaded_models.find(key_hash); dup_it != kml_models_context.loaded_models.end()) {
    php_warning("model_name '%s' is duplicated\n", kml.model_name.c_str());
  }

  kml_models_context.loaded_models[key_hash] = std::move(kml);
}

static void traverse_kml_dir(const std::string& path) {
  struct stat st {};
  if (stat(path.c_str(), &st) != 0) {
    return;
  }
  const bool is_directory = S_ISDIR(st.st_mode);
  if (!is_directory && ends_with(path.c_str(), ".kml")) {
    load_kml_file(path);
    return;
  }

  if (is_directory) {
    if (std::unique_ptr<DIR, int (*)(DIR*)> dir{opendir(path.c_str()), closedir}) {
      struct dirent* iter = nullptr;
      while ((iter = readdir(dir.get()))) {
        if (strcmp(iter->d_name, ".") == 0 || strcmp(iter->d_name, "..") == 0) {
          continue;
        }
        traverse_kml_dir(path + "/" + iter->d_name);
      }
    } else {
      php_warning("cannot read %s (%s)\n", path.c_str(), std::strerror(errno));
    }
  }
}

void init_kphp_ml_runtime_in_master() {
  const auto& kml_models_context = KmlModelsContext::get();

  if (kml_models_context.kml_directory == nullptr || kml_models_context.kml_directory[0] == '\0') {
    return;
  }

  traverse_kml_dir(kml_models_context.kml_directory);

  php_info("loaded %d kml models from %s\n", static_cast<int>(kml_models_context.loaded_models.size()), kml_models_context.kml_directory);
}

void init_kphp_ml_runtime_in_worker() {
  KmlInferenceContext::get().mutable_buffer_in_worker = kphp::memory::platform_allocator<char>{}.allocate(KmlModelsContext::get().max_mutable_buffer_size);
}

char* kphp_ml_get_mutable_buffer_in_current_worker() {
  return KmlInferenceContext::get().mutable_buffer_in_worker;
}

const kphp_ml::MLModel* kphp_ml_find_loaded_model_by_name(const string& model_name) {
  uint64_t key_hash = string_hash(model_name.c_str(), model_name.size());

  const auto& kml_models_context = KmlModelsContext::get();

  auto found_it = kml_models_context.loaded_models.find(key_hash);
  return found_it == kml_models_context.loaded_models.end() ? nullptr : &found_it->second;
}
