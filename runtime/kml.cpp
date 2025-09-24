#include "runtime-common/stdlib/kml/kml-file-api.h"
#include "runtime-common/stdlib/kml/kml-inference-context.h"
#include "runtime-common/stdlib/kml/kml-models-context.h"

namespace {
KmlInferenceContext kml_inference_context;
KmlModelsContext kml_models_context;
} // namespace

KmlInferenceContext& KmlInferenceContext::get() noexcept {
  return kml_inference_context;
}

const KmlModelsContext& KmlModelsContext::get() noexcept {
  return kml_models_context;
}

KmlModelsContext& KmlModelsContext::get_mutable() noexcept {
  return kml_models_context;
}

struct LibcFileReader final : public kphp_ml::IFileReader, private vk::not_copyable {
private:
  FILE* file_{nullptr};

public:
  explicit LibcFileReader(vk::string_view filename) noexcept
      : file_{fopen(filename.data(), "rb")} {}

  ~LibcFileReader() noexcept final {
    if (file_) {
      std::ignore = fclose(file_);
    }
  }

  void read(char* dest, size_t sz) noexcept final {
    int read_cnt = fread(dest, 1, sz, file_);
    assert(read_cnt == sz);
  }

  bool is_eof() const noexcept final {
    return file_ ? feof(file_) != 0 : true;
  }
};

struct LibcDirTraverser final : public kphp_ml::IDirTraverser, private vk::not_copyable {
  callback_t callback;

  explicit LibcDirTraverser(callback_t callback) noexcept
      : callback{callback} {}

  bool is_dir([[maybe_unused]] const kphp_ml::stl::string& path) {
    struct stat st {};
    if (stat(path.c_str(), &st) != 0) {
      return false;
    }
    return S_ISDIR(st.st_mode);
  }

  void traverse(const kphp_ml::stl::string& cur_path) final {
    php_info("[kml] Iterating %s\n", cur_path.c_str());
    struct stat st {};
    if (stat(cur_path.c_str(), &st) != 0) {
      php_info("[kml] Bad stat %s\n", cur_path.c_str());
      return;
    }

    if (!S_ISDIR(st.st_mode)) {
      php_info("[kml] Calling callback %s\n", cur_path.c_str());
      callback(cur_path);
      return;
    }
    if (std::unique_ptr<DIR, int (*)(DIR*)> dir{opendir(cur_path.c_str()), closedir}) {
      struct dirent* iter = nullptr;
      while ((iter = readdir(dir.get()))) {
        if (strcmp(iter->d_name, ".") == 0 || strcmp(iter->d_name, "..") == 0) {
          continue;
        }
        traverse(cur_path + "/" + iter->d_name);
      }
    } else {
      php_warning("cannot read %s (%s)\n", cur_path.c_str(), std::strerror(errno));
    }
  }
};

namespace kphp_ml {

kphp_ml::stl::unique_ptr_platform<kphp_ml::IFileReader> get_file_reader(vk::string_view file_name) {
  return static_cast<kphp_ml::stl::unique_ptr_platform<kphp_ml::IFileReader>>(kphp_ml::stl::make_unique<LibcFileReader>(file_name).release());
}
kphp_ml::stl::unique_ptr_platform<IDirTraverser> get_dir_traverser(IDirTraverser::callback_t callback) {
  return static_cast<kphp_ml::stl::unique_ptr_platform<IDirTraverser>>(kphp_ml::stl::make_unique<LibcDirTraverser>(callback).release());
}
} // namespace kphp_ml
