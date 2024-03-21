#include "runtime/kphp_ml/kphp_ml_init.h"

#include <exception>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "runtime/kphp_ml/kphp_ml.h"
#include "runtime/kphp_ml/kml-files-reader.h"
#include "common/kprintf.h"


const char *KmlDirectory = nullptr;
std::unordered_map<uint64_t, kphp_ml::MLModel> LoadedModels;
unsigned int MaxMutableBufferSize = 0;

char *MutableBufferInWorker = nullptr;

void init_kphp_ml_runtime_in_master() {
  if (KmlDirectory == nullptr || KmlDirectory[0] == '\0') {
    return;
  }

  for (auto iter = std::filesystem::recursive_directory_iterator(KmlDirectory), end = std::filesystem::recursive_directory_iterator(); iter != end; ++iter) {
    if (iter->path().extension() != ".kml") {
      continue;
    }

    kphp_ml::MLModel kml;
    try {
      kml = kml_file_read(iter->path().c_str());
    } catch (const std::exception &ex) {
      kprintf("error reading %s: %s\n", iter->path().c_str(), ex.what());
      continue;
    }

    unsigned int buffer_size = kml.calculate_mutable_buffer_size();
    MaxMutableBufferSize = std::max(MaxMutableBufferSize, (buffer_size + 3) & -4);

    uint64_t key_hash = string_hash(kml.model_name.c_str(), kml.model_name.size());
    if (auto dup_it = LoadedModels.find(key_hash); dup_it != LoadedModels.end()) {
      kprintf("warning: model_name '%s' is duplicated\n", kml.model_name.c_str());
    }

    LoadedModels[key_hash] = std::move(kml);
  }

  kprintf("loaded %d kml models from %s\n", static_cast<int>(LoadedModels.size()), KmlDirectory);
}

void init_kphp_ml_runtime_in_worker() {
  MutableBufferInWorker = new char[MaxMutableBufferSize];
}

char *kphp_ml_get_mutable_buffer_in_current_worker() {
  return MutableBufferInWorker;
}

const kphp_ml::MLModel *kphp_ml_find_loaded_model_by_name(const string &model_name) {
  uint64_t key_hash = string_hash(model_name.c_str(), model_name.size());
  auto found_it = LoadedModels.find(key_hash);

  return found_it == LoadedModels.end() ? nullptr : &found_it->second;
}
