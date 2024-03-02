#include "runtime/ml/init.h"

#include <filesystem>

#include "common/kprintf.h"

#include "runtime/ml/kml-files-reader.h"

template<class>
[[maybe_unused]] static inline constexpr bool always_false_v = false;

static int align_to(int size, int align) {
  return (size + align - 1) / align * align;
}

void init_ml_runtime() {
  if (KmlDirPath.empty()) {
    return;
  }

  namespace fs = std::filesystem;
  /**
   * I use recursive iterators to support something like this
   * /kml/feed/model_feed.kml
   * /kml/ads/model_ads.kml
   */
  PredictionBufferSize = 0;
  for (auto iter = fs::recursive_directory_iterator(KmlDirPath), end = fs::recursive_directory_iterator(); iter != end; ++iter) {
    if (iter->path().extension() == ".kml") {
      auto model = kml_file_read(iter->path().c_str());

      if (LoadedModels.count(model.model_name) == 0) {
        kprintf("kml-model with name \"%s\" successfully loaded", model.model_name.c_str());
      } else {
        kprintf("kml-model with name \"%s\" is already loaded; previous one will be used", model.model_name.c_str());
      }

      auto cur_model_prediction_size = std::visit(
        [](auto &impl) -> size_t {
          using impl_type = std::decay_t<decltype(impl)>;
          if constexpr (std::is_same_v<ml_internals::XgbModel, impl_type>) {
            return 2 * ml_internals::BATCH_SIZE_XGB * impl.num_features_present * 2 * sizeof(float);
          } else if constexpr (std::is_same_v<ml_internals::CbModel, impl_type>) {
            return align_to(sizeof(char) * impl.binary_feature_count, sizeof(int)) + sizeof(int) * impl.cat_feature_count
                   + sizeof(std::pair<int, int>) * impl.cat_feature_count + sizeof(float) * impl.model_ctrs.used_model_ctrs_count;
          } else {
            static_assert(always_false_v<impl_type>, "Cannot calculate memory required for a model to predict: unknown kml type");
          }
        },
        model.impl);
      PredictionBufferSize = std::max(PredictionBufferSize, cur_model_prediction_size);

      LoadedModels[model.model_name] = std::move(model);
    }
  }
}