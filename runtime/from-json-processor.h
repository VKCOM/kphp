// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

#include "runtime/kphp_core.h"
#include "runtime/json-functions.h"
#include "runtime/json-processor-utils.h"

template<class Tag>
class FromJsonVisitor {
public:
  explicit FromJsonVisitor(const mixed &json, bool flatten_class, JsonPath &json_path) noexcept
    : json_(json)
    , flatten_class_(flatten_class)
    , json_path_ (json_path) {}

  template<class T>
  void operator()(const char *key, T &value, bool required = false) noexcept {
    if (!error_.empty()) {
      return;
    }
    if (flatten_class_) {
      do_set(value, json_);
      return;
    }
    json_path_.enter(key);
    const auto *json_value = json_.as_array().find_value(string{key});
    if (required && !json_value) {
      error_.append("absent required field ");
      error_.append(json_path_.to_string());
    }
    if (json_value) {
      do_set(value, *json_value);
    }
    json_path_.leave();
  }

  bool has_error() const noexcept { return !error_.empty(); }
  const string &get_error() const noexcept { return error_; }

  static const char *get_json_obj_magic_key() noexcept {
    return "__json_obj_magic";
  }

private:
  [[gnu::noinline]] void on_input_type_mismatch(const mixed &json) noexcept {
     error_.assign("unexpected type ");
     if (json.is_array()) {
       error_.append(json.as_array().is_vector() ? "array" : "object");
     } else {
       error_.append(json.get_type_str());
     }
     error_.append(" for key ");
     error_.append(json_path_.to_string());
  }

  void do_set(bool &value, const mixed &json) noexcept {
    if (!json.is_bool()) {
      on_input_type_mismatch(json);
      return;
    }
    value = json.as_bool();
  }

  void do_set(std::int64_t &value, const mixed &json) noexcept {
    if (!json.is_int()) {
      on_input_type_mismatch(json);
      return;
    }
    value = json.as_int();
  }

  void do_set(double &value, const mixed &json) noexcept {
    if (!json.is_float() && !json.is_int()) {
      on_input_type_mismatch(json);
      return;
    }
    value = json.is_int() ? json.as_int() : json.as_double();
  }

  void do_set(string &value, const mixed &json) noexcept {
    if (!json.is_string()) {
      on_input_type_mismatch(json);
      return;
    }
    value = json.as_string();
  }

  void do_set(JsonRawString &value, const mixed &json) noexcept {
    static_SB.clean();
    if (!impl_::JsonEncoder{0, false, get_json_obj_magic_key()}.encode(json)) {
      error_.append("failed to decode @kphp-json raw_string field ");
      error_.append(json_path_.to_string());
      return;
    }
    value.str = static_SB.str();
  }

  template<class T>
  void do_set(Optional<T> &value, const mixed &json) noexcept {
    if (json.is_null()) {
      value = Optional<bool>{};
      return;
    }
    do_set(value.ref(), json);
  }

  template<class I>
  void do_set(class_instance<I> &klass, const mixed &json) noexcept;

  // just don't fail compilation with empty untyped arrays
  void do_set(array<Unknown> &/*array*/, const mixed &/*json*/) noexcept {}

  template<class T>
  void do_set_array(array<T> &array, const mixed &json) noexcept {
    const auto &json_array = json.as_array();
    const auto array_size = json_array.size();
    // overwrite (but not just merge) array data
    array.clear();
    array.reserve(array_size.size, array_size.is_vector);

    json_path_.enter(nullptr);
    for (const auto it : json_array) {
      auto json_key = it.get_key();
      if (json_key.is_string() && !strcmp(json_key.as_string().c_str(), get_json_obj_magic_key())) {
        continue; // don't deserialize magic
      }
      do_set(array[json_key], it.get_value());
    }
    json_path_.leave();
  }

  template<class T>
  void do_set(array<T> &array, const mixed &json) noexcept {
    if (json.is_array()) {
      do_set_array(array, json);
    } else {
      on_input_type_mismatch(json);
    }
  }

  void do_set(mixed &value, const mixed &json) noexcept {
    if (json.is_array()) {
      array<mixed> array;
      do_set(array, json);
      value = std::move(array);
    } else {
      value = json;
    }
  }

  string error_;
  const mixed &json_;
  bool flatten_class_{false};
  JsonPath& json_path_;
};

template<class I, class Tag>
class_instance<I> from_json_impl(const mixed &json, JsonPath &json_path) noexcept {
  class_instance<I> instance;
  if constexpr (std::is_empty_v<I>) {
    instance.empty_alloc();
  } else {
    instance.alloc();
    FromJsonVisitor<Tag> visitor{json, impl_::IsJsonFlattenClass<I>::value, json_path};
    instance.get()->accept(visitor);
    if (visitor.has_error()) {
      JsonEncoderError::msg.append(visitor.get_error());
      return {};
    }
  }
  if constexpr (impl_::HasClassWakeupMethod<I>::value) {
    instance.get()->wakeup(instance);
  }
  return JsonEncoderError::msg.empty() ? instance : class_instance<I>{};
}

template<class Tag>
template<class I>
void FromJsonVisitor<Tag>::do_set(class_instance<I> &klass, const mixed &json) noexcept {
  if constexpr (!impl_::IsJsonFlattenClass<I>::value) {
    if (json.is_null()) {
      return;
    }
    if (!json.is_array() || json.as_array().is_vector()) {
      on_input_type_mismatch(json);
      return;
    }
  }
  klass = from_json_impl<I, Tag>(json, json_path_);
}

template<class ClassName, class Tag>
ClassName f$JsonEncoder$$from_json_impl(Tag /*tag*/, const string &json_string, const string &/*class_mame*/) noexcept {
  JsonEncoderError::msg = {};

  auto [json, success] = json_decode(json_string, FromJsonVisitor<Tag>::get_json_obj_magic_key());

  if (!success) {
    JsonEncoderError::msg.append(json_string.empty() ? "provided empty json string" : "failed to parse json string");
    return {};
  }
  if constexpr (!impl_::IsJsonFlattenClass<typename ClassName::ClassType>::value) {
    if (!json.is_array() || json.as_array().is_vector()) {
      JsonEncoderError::msg.append("root element of json string must be an object type, got ");
      JsonEncoderError::msg.append(json.get_type_c_str());
      return {};
    }
  }

  JsonPath json_path;
  return from_json_impl<typename ClassName::ClassType, Tag>(json, json_path);
}

string f$JsonEncoder$$getLastError() noexcept;
