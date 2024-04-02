#include "runtime/kphp_ml/kphp_ml_init.h"

#include <dirent.h>
#include <exception>
#include <string>
#include <unordered_map>

#include "common/kprintf.h"
#include "runtime/kphp_ml/kml-files-reader.h"

const char *KmlDirectory = nullptr;
std::unordered_map<uint64_t, kphp_ml::MLModel> LoadedModels;
unsigned int MaxMutableBufferSize = 0;

char *MutableBufferInWorker = nullptr;

static bool ends_with(const char *str, const char *suffix) {
  size_t len_str = strlen(str);
  size_t len_suffix = strlen(suffix);

  return len_suffix <= len_str && strncmp(str + len_str - len_suffix, suffix, len_suffix) == 0;
}

void init_kphp_ml_runtime_in_master() {
  if (KmlDirectory == nullptr || KmlDirectory[0] == '\0') {
    return;
  }

  DIR *dir = opendir(KmlDirectory);
  struct dirent *iter;

  while ((iter = readdir(dir))) {
    if (!ends_with(iter->d_name, ".kml")) {
      continue;
    }

    kphp_ml::MLModel kml;
    try {
      kml = kml_file_read(std::string(KmlDirectory) + "/" + iter->d_name);
    } catch (const std::exception &ex) {
      kprintf("error reading %s: %s\n", iter->d_name, ex.what());
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

  closedir(dir);
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
