// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/json-functions.h"
#include "runtime-common/stdlib/string/json-processor-utils.h"
#include "runtime-common/stdlib/string/json-writer.h"
#include "runtime-common/stdlib/string/string-context.h"

template <class Tag>
class ToJsonVisitor {
public:
  explicit ToJsonVisitor(impl_::JsonWriter &writer, bool flatten_class, std::size_t depth) noexcept
    : writer_(writer)
    , flatten_class_(flatten_class)
    , depth_(flatten_class ? depth : ++depth)
  {}

  template<class T>
  void operator()(const char *key, const T &value, bool array_as_hashmap = false) noexcept {
    array_as_hashmap_ = array_as_hashmap;
    if (!flatten_class_) {
      writer_.write_key(key);
    }
    process_impl(value);
  }

  void set_precision(std::size_t precision) noexcept {
    writer_.set_double_precision(precision);
  }

  void restore_precision() noexcept {
    writer_.set_double_precision(0);
  }

private:
  template<class T>
  void process_impl(const T &value) noexcept {
    add_value(value);
  }

  template<class T>
  void process_impl(const Optional<T> &value) noexcept {
    auto process_impl_lambda = [this](const auto &val) { return this->process_impl(val); };
    call_fun_on_optional_value(process_impl_lambda, value);
  }

  template<class I>
  void process_impl(const class_instance<I> &instance) noexcept;

  template<class T>
  void process_vector(const array<T> &array) noexcept {
    writer_.start_array();
    for (const auto &elem : array) {
      process_impl(elem.get_value());
    }
    writer_.end_array();
  }

  template<class T>
  void process_map(const array<T> &array) noexcept {
    writer_.start_object();
    for (const auto &elem : array) {
      const auto &key = elem.get_key().to_string();
      writer_.write_key({key.c_str(), key.size()}, true);
      process_impl(elem.get_value());
    }
    writer_.end_object();
  }

  template<class T>
  void process_impl(const array<T> &array) noexcept {
    //use array_as_hashmap_ only for top level of current array field, don't propagate it further
    bool as_hashmap = std::exchange(array_as_hashmap_, false);
    (array.is_vector() || array.is_pseudo_vector()) && !as_hashmap ? process_vector(array) : process_map(array);
  }

  // support of array<Unknown> compilation
  void process_impl(const Unknown &/*elem*/) noexcept {}

  void process_impl(const mixed &value) noexcept {
    switch (value.get_type()) {
      case mixed::type::NUL:
        add_null_value();
        break;
      case mixed::type::BOOLEAN:
        add_value(value.as_bool());
        break;
      case mixed::type::INTEGER:
        add_value(value.as_int());
        break;
      case mixed::type::FLOAT:
        add_value(value.as_double());
        break;
      case mixed::type::STRING:
        add_value(value.as_string());
        break;
      case mixed::type::ARRAY:
        process_impl(value.as_array());
        break;
      case mixed::type::OBJECT:
        php_warning("Objects (%s) in mixed cannot be written in JSON", value.get_type_or_class_name());
        break;
    }
  }

  void add_value(const string &value) noexcept {
    writer_.write_string(value);
  }

  void add_value(const JsonRawString &value) noexcept {
    writer_.write_raw_string(value.str);
  }

  void add_value(std::int64_t value) noexcept {
    writer_.write_int(value);
  }

  void add_value(bool value) noexcept {
    writer_.write_bool(value);
  }

  void add_value(double value) noexcept {
    writer_.write_double(value);
  }

  void add_null_value() noexcept {
    writer_.write_null();
  }

  impl_::JsonWriter &writer_;
  const bool flatten_class_{false};
  bool array_as_hashmap_{false};
  std::size_t depth_{0};
};

template<class Tag, class T, class U = Unknown>
void to_json_impl(const class_instance<T> &klass, impl_::JsonWriter &writer, const array<U> &more, std::size_t depth = 0) noexcept {
  if (depth > 64) {
    StringLibContext::get().last_json_processor_error.assign("allowed depth=64 of json object exceeded");
    return;
  }

  if (klass.is_null()) {
    writer.write_null();
    return;
  }
  constexpr bool flatten_class = impl_::IsJsonFlattenClass<T>::value;
  if constexpr (!flatten_class) {
    writer.start_object();
  }

  ToJsonVisitor<Tag> visitor{writer, flatten_class, depth};
  if constexpr (!std::is_empty_v<T>) {
    klass.get()->accept(visitor);
  }

  for (auto elem : more) {
    const auto &key = elem.get_key().to_string();
    visitor(key.c_str(), elem.get_value());
  }

  if constexpr (!flatten_class) {
    writer.end_object();
  }
}

template<class Tag>
template<class I>
void ToJsonVisitor<Tag>::process_impl(const class_instance<I> &instance) noexcept {
  to_json_impl<Tag>(instance, writer_, {}, depth_);
}

template<class Tag, class T, class U = Unknown>
string f$JsonEncoder$$to_json_impl(Tag /*tag*/, const class_instance<T> &klass, int64_t flags = 0, const array<U> &more = {}) noexcept {
  const bool has_unsupported_option = static_cast<bool>(flags & ~JSON_AVAILABLE_FLAGS_TYPED);
  if (unlikely(has_unsupported_option)) {
    php_warning("Wrong parameter flags = %" PRIi64 " in function JsonEncoder::encode", flags);
  }
  auto &error_msg = StringLibContext::get().last_json_processor_error;
  error_msg = {};

  impl_::JsonWriter writer{(flags & JSON_PRETTY_PRINT) > 0, (flags & JSON_PRESERVE_ZERO_FRACTION) > 0};
  to_json_impl<Tag>(klass, writer, more);

  if (!error_msg.empty()) {
    return {};
  }
  if (!writer.is_complete()) {
    // unexpected internal error
    php_warning("JsonEncoder internal error: %s", writer.get_error().c_str());
    return {};
  }
  return writer.get_final_json();
}
