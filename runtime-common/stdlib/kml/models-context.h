// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "common/php-functions.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/kml/model.h"

template<class DirTraverser, template<class> class Allocator>
class KmlModelsContext final : private vk::not_copyable {
  size_t m_buffer_size{};
  kphp::stl::unordered_map<uint64_t, kphp::kml::model<Allocator>, Allocator> m_loaded_models;

public:
  KmlModelsContext() noexcept = default;

  void init(std::string_view kml_dir_path) noexcept {
    auto opt_dir_traverser{DirTraverser::create(kml_dir_path)};
    if (!opt_dir_traverser) {
      return;
    }

    auto dir_traverser{*std::move(opt_dir_traverser)};
    for (auto opt_file_reader{dir_traverser.next()}; opt_file_reader; opt_file_reader = dir_traverser.next()) {
      auto opt_model{kphp::kml::model<Allocator>::load(*std::move(opt_file_reader))};
      if (!opt_model) {
        continue;
      }

      auto model{*std::move(opt_model)};
      const auto key_hash{string_hash(model.name().data(), model.name().size())};
      if (auto it{m_loaded_models.find(key_hash)}; it != m_loaded_models.end()) {
        php_warning("failed to load duplicated model: name -> %s", model.name().data());
        continue;
      }

      m_buffer_size = std::max(m_buffer_size, model.mutable_buffer_size());
      m_loaded_models.emplace(key_hash, std::move(model));
    }
  }

  size_t max_buffer_size() const noexcept {
    return m_buffer_size;
  }

  std::optional<std::reference_wrapper<const kphp::kml::model<Allocator>>> find_model(std::string_view name) const noexcept {
    if (auto it{m_loaded_models.find(string_hash(name.data(), name.size()))}; it != m_loaded_models.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  const auto& models() const noexcept {
    return m_loaded_models;
  }

  static const KmlModelsContext& get() noexcept;
  static KmlModelsContext& get_mutable() noexcept;
};
