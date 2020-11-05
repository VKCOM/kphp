// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

template<class T>
std::enable_if_t<std::is_empty<T>{}, array<mixed>> f$instance_to_array(const class_instance<T> &);

template<class T>
std::enable_if_t<!std::is_empty<T>{}, array<mixed>> f$instance_to_array(const class_instance<T> &c);

class InstanceToArrayVisitor {
public:
  array<mixed> flush_result() {
    return std::move(result_);
  }

  template<typename T>
  void operator()(const char *field_name, const T &value) {
    process_impl(field_name, value);
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
    add_value(field_name, instance.is_null() ? mixed{} : f$instance_to_array(instance));
  }

  template<class ...Args>
  void process_impl(const char *field_name, const std::tuple<Args...> &value) {
    InstanceToArrayVisitor tuple_processor;
    tuple_processor.result_.reserve(sizeof...(Args), 0, true);

    process_tuple(value, tuple_processor);
    add_value(field_name, tuple_processor.flush_result());
  }

  template<size_t ...Is, typename ...T>
  void process_impl(const char *field_name, const shape<std::index_sequence<Is...>, T...> &value) {
    InstanceToArrayVisitor shape_processor;
    shape_processor.result_.reserve(sizeof...(Is), 0, true);

    process_shape(value, shape_processor);
    add_value(field_name, shape_processor.flush_result());
  }

  template<size_t Index = 0, class ...Args>
  std::enable_if_t<Index != sizeof...(Args)> process_tuple(const std::tuple<Args...> &value, InstanceToArrayVisitor &tuple_visitor) {
    tuple_visitor.process_impl("", std::get<Index>(value));
    process_tuple<Index + 1>(value, tuple_visitor);
  }

  template<size_t Index = 0, class ...Args>
  std::enable_if_t<Index == sizeof...(Args)> process_tuple(const std::tuple<Args...> &, InstanceToArrayVisitor &/*tuple_processor*/) {
  }

  template<size_t ...Is, typename ...T>
  void process_shape(const shape<std::index_sequence<Is...>, T...> &value, InstanceToArrayVisitor &shape_visitor) {
    // shape doesn't have key names at runtime, that's why result will be a vector-array (whereas associative in PHP)
    std::initializer_list<int32_t>{((void)shape_visitor.process_impl("", value.template get<Is>()), 0)...};
  }

  template<class T>
  void add_value(const char *field_name, T &&value) {
    if (field_name[0] != '\0') {
      result_.set_value(string{field_name}, std::forward<T>(value));
    } else {
      result_.push_back(std::forward<T>(value));
    }
  }

private:
  array<mixed> result_;
};

template<class T>
std::enable_if_t<std::is_empty<T>{}, array<mixed>> f$instance_to_array(const class_instance<T> &) {
  return {};
}

template<class T>
std::enable_if_t<!std::is_empty<T>{}, array<mixed>> f$instance_to_array(const class_instance<T> &c) {
  if (c.is_null()) {
    return {};
  }
  InstanceToArrayVisitor visitor;
  c.get()->accept(visitor);
  return visitor.flush_result();
}
