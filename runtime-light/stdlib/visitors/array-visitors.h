// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/components/kphp/state/image-state.h"

template<>
inline const ShapeKeyDemangle<kphp::memory::script_allocator>& ShapeKeyDemangle<kphp::memory::script_allocator>::get() noexcept {
  return ImageState::get().shape_key_demangler;
}

class ToArrayVisitor {
public:
  explicit ToArrayVisitor(bool with_class_names) noexcept
      : with_class_names_(with_class_names) {}

  array<mixed> flush_result() && noexcept {
    return std::move(result_);
  }

  template<typename T>
  void operator()(std::string_view field_name, const T& value) noexcept {
    this->process_impl(field_name, value);
  }

  template<class... Args, std::size_t... Indexes>
  void process_tuple(const std::tuple<Args...>& tuple, std::index_sequence<Indexes...> /*indexes*/) noexcept {
    (this->process_impl("", std::get<Indexes>(tuple)), ...);
  }

  template<size_t... Is, typename... T>
  void process_shape(const shape<std::index_sequence<Is...>, T...>& shape) noexcept {
    const auto& demangler{ShapeKeyDemangle<kphp::memory::script_allocator>::get()};
    (this->process_impl(demangler.get_key_by(Is).data(), shape.template get<Is>()), ...);
  }

  template<class I>
  void process_instance(const class_instance<I>& instance) noexcept {
    static constexpr std::string_view class_name_key{"__class_name"};
    if (instance.is_null()) {
      return;
    }

    if constexpr (!std::is_empty_v<I>) {
      void* instance_raw_ptr{instance.get()->get_instance_data_raw_ptr()};
      if (active_instances_set.contains(instance_raw_ptr)) {
        return;
      }
      active_instances_set.emplace(instance_raw_ptr);
      instance.get()->accept(*this);
      active_instances_set.erase(instance_raw_ptr);
    }

    if (with_class_names_) {
      result_.set_value(string{class_name_key.data(), static_cast<string::size_type>(class_name_key.size())}, string{instance.get_class()});
    }
  }

  template<class T>
  void process_array(const array<T>& value) noexcept {
    for (auto it{value.begin()}; it != value.end(); ++it) {
      this->process_impl("", it.get_value());
      result_.set_value(it.get_key(), result_.pop());
    }
  }

private:
  template<class T>
  void process_impl(std::string_view field_name, const T& value) noexcept {
    this->add_value(field_name, value);
  }

  template<typename T>
  void process_impl(std::string_view field_name, const Optional<T>& value) noexcept {
    auto process_impl_lambda{[this, field_name](const auto& v) { return this->process_impl(field_name, v); }};
    call_fun_on_optional_value(std::move(process_impl_lambda), value);
  }

  template<class T>
  void process_impl(std::string_view field_name, const array<T>& value) noexcept {
    array<mixed> process_result{std::exchange(result_, array<mixed>{})};
    auto array_size{value.size()};
    result_.reserve(array_size.size, array_size.is_vector);
    this->process_array(value);
    std::swap(process_result, result_);

    this->add_value(field_name, std::move(process_result));
  }

  template<class I>
  void process_impl(std::string_view field_name, const class_instance<I>& instance) noexcept {
    if (instance.is_null()) {
      this->add_value(field_name, mixed{});
      return;
    }

    array<mixed> process_result{std::exchange(result_, array<mixed>{})};
    this->process_instance(instance);
    std::swap(process_result, result_);

    this->add_value(field_name, std::move(process_result));
  }

  template<class... Args>
  void process_impl(std::string_view field_name, const std::tuple<Args...>& value) noexcept {
    array<mixed> process_result{std::exchange(result_, array<mixed>{})};
    result_.reserve(sizeof...(Args), true);
    this->process_tuple(value, std::index_sequence_for<Args...>{});
    std::swap(process_result, result_);

    this->add_value(field_name, std::move(process_result));
  }

  template<size_t... Is, typename... T>
  void process_impl(std::string_view field_name, const shape<std::index_sequence<Is...>, T...>& value) noexcept {
    array<mixed> process_result{std::exchange(result_, array<mixed>{})};
    result_.reserve(sizeof...(Is), true);
    this->process_shape(value);
    std::swap(process_result, result_);

    this->add_value(field_name, std::move(process_result));
  }

  template<class T>
  void add_value(std::string_view field_name, T&& value) noexcept {
    if (!field_name.empty()) {
      result_.set_value(string{field_name.data(), static_cast<string::size_type>(field_name.size())}, std::forward<T>(value));
    } else {
      result_.push_back(std::forward<T>(value));
    }
  }

  array<mixed> result_;
  bool with_class_names_{};
  kphp::stl::unordered_set<void*, kphp::memory::script_allocator> active_instances_set;
};

template<class T>
array<mixed> f$to_array_debug(const class_instance<T>& klass, bool with_class_names = false) noexcept {
  ToArrayVisitor visitor{with_class_names};
  visitor.process_instance(klass);
  return std::move(visitor).flush_result();
}

template<class... Args>
array<mixed> f$to_array_debug(const std::tuple<Args...>& tuple, bool with_class_names = false) noexcept {
  ToArrayVisitor visitor{with_class_names};
  visitor.process_tuple(tuple, std::index_sequence_for<Args...>{});
  return std::move(visitor).flush_result();
}

template<size_t... Indexes, typename... T>
array<mixed> f$to_array_debug(const shape<std::index_sequence<Indexes...>, T...>& shape, bool with_class_names = false) noexcept {
  ToArrayVisitor visitor{with_class_names};
  visitor.process_shape(shape);
  return std::move(visitor).flush_result();
}

template<class T>
array<mixed> f$instance_to_array(const class_instance<T>& klass, bool with_class_names = false) noexcept {
  return f$to_array_debug(klass, with_class_names);
}

template<class... Args>
array<mixed> f$instance_to_array(const std::tuple<Args...>& tuple, bool with_class_names = false) noexcept {
  return f$to_array_debug(tuple, with_class_names);
}

template<size_t... Indexes, typename... T>
array<mixed> f$instance_to_array(const shape<std::index_sequence<Indexes...>, T...>& shape, bool with_class_names = false) noexcept {
  return f$to_array_debug(shape, with_class_names);
}
