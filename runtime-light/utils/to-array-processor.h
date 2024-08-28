// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "runtime-core/runtime-core.h"

class ShapeKeyDemangle : vk::not_copyable {
public:
  friend class vk::singleton<ShapeKeyDemangle>;

  void init(std::unordered_map<std::int64_t, std::string_view> &&shape_keys_storage) noexcept {
    inited_ = true;
    shape_keys_storage_ = std::move(shape_keys_storage);
  }

  std::string_view get_key_by(std::int64_t tag) const noexcept {
    php_assert(inited_);
    auto key_it = shape_keys_storage_.find(tag);
    php_assert(key_it != shape_keys_storage_.end());
    return key_it->second;
  }

private:
  ShapeKeyDemangle() = default;

  bool inited_{false};
  std::unordered_map<std::int64_t, std::string_view> shape_keys_storage_;
};

class ToArrayVisitor {
public:
  explicit ToArrayVisitor(bool with_class_names)
    : with_class_names_(with_class_names) {}

  array<mixed> flush_result() && noexcept {
    return std::move(result_);
  }

  template<typename T>
  void operator()(const char *field_name, const T &value) {
    process_impl(field_name, value);
  }

  template<class... Args, std::size_t... Indexes>
  static void process_tuple(const std::tuple<Args...> &tuple, ToArrayVisitor &visitor, std::index_sequence<Indexes...> /*indexes*/) {
    (visitor.process_impl("", std::get<Indexes>(tuple)), ...);
  }

  template<size_t... Is, typename... T>
  static void process_shape(const shape<std::index_sequence<Is...>, T...> &shape, ToArrayVisitor &visitor) {
    auto &demangler = vk::singleton<ShapeKeyDemangle>::get();
    (visitor.process_impl(demangler.get_key_by(Is).data(), shape.template get<Is>()), ...);
  }

private:
  template<class T>
  void process_impl(const char *field_name, const T &value) {
    add_value(field_name, value);
  }

  template<typename T>
  void process_impl(const char *field_name, const Optional<T> &value) {
    auto process_impl_lambda = [this, field_name](const auto &v) { return this->process_impl(field_name, v); };
    call_fun_on_optional_value(process_impl_lambda, value);
  }

  template<class T>
  void process_impl(const char *field_name, const array<T> &value) {
    array<mixed> converted_value(value.size());
    for (auto it = value.begin(); it != value.end(); ++it) {
      process_impl("", it.get_value());
      converted_value.set_value(it.get_key(), result_.pop());
    }

    add_value(field_name, converted_value);
  }

  template<class I>
  void process_impl(const char *field_name, const class_instance<I> &instance) {
    add_value(field_name, instance.is_null() ? mixed{} : f$to_array_debug(instance, with_class_names_));
  }

  template<class ...Args>
  void process_impl(const char *field_name, const std::tuple<Args...> &value) {
    ToArrayVisitor tuple_processor{with_class_names_};
    tuple_processor.result_.reserve(sizeof...(Args), true);

    process_tuple(value, tuple_processor, std::index_sequence_for<Args...>{});
    add_value(field_name, std::move(tuple_processor).flush_result());
  }

  template<size_t ...Is, typename ...T>
  void process_impl(const char *field_name, const shape<std::index_sequence<Is...>, T...> &value) {
    ToArrayVisitor shape_processor{with_class_names_};
    shape_processor.result_.reserve(sizeof...(Is), true);

    process_shape(value, shape_processor);
    add_value(field_name, std::move(shape_processor).flush_result());
  }

  template<class T>
  void add_value(const char *field_name, T &&value) {
    if (field_name[0] != '\0') {
      result_.set_value(string{field_name}, std::forward<T>(value));
    } else {
      result_.push_back(std::forward<T>(value));
    }
  }

  array<mixed> result_;
  bool with_class_names_{false};
};

template<class T>
array<mixed> f$to_array_debug(const class_instance<T> &klass, bool with_class_names = false) {
  array<mixed> result;
  if (klass.is_null()) {
    return result;
  }

  if constexpr (!std::is_empty_v<T>) {
    ToArrayVisitor visitor{with_class_names};
    klass.get()->accept(visitor);
    result = std::move(visitor).flush_result();
  }

  if (with_class_names) {
    result.set_value(string("__class_name"), string(klass.get_class()));
  }
  return result;
}

template<class... Args>
array<mixed> f$to_array_debug(const std::tuple<Args...> &tuple, bool with_class_names = false) {
  ToArrayVisitor visitor{with_class_names};
  ToArrayVisitor::process_tuple(tuple, visitor, std::index_sequence_for<Args...>{});
  return std::move(visitor).flush_result();
}

template<size_t... Indexes, typename... T>
array<mixed> f$to_array_debug(const shape<std::index_sequence<Indexes...>, T...> &shape, bool with_class_names = false) {
  ToArrayVisitor visitor{with_class_names};
  ToArrayVisitor::process_shape(shape, visitor);
  return std::move(visitor).flush_result();
}

template<class T>
array<mixed> f$instance_to_array(const class_instance<T> &klass, bool with_class_names = false) {
  return f$to_array_debug(klass, with_class_names);
}

template<class... Args>
array<mixed> f$instance_to_array(const std::tuple<Args...> &tuple, bool with_class_names = false) {
  return f$to_array_debug(tuple, with_class_names);
}

template<size_t... Indexes, typename... T>
array<mixed> f$instance_to_array(const shape<std::index_sequence<Indexes...>, T...> &shape, bool with_class_names = false) {
  return f$to_array_debug(shape, with_class_names);
}
