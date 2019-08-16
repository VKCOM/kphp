#pragma once

#include "runtime/kphp_core.h"

template<class T>
array<var> f$instance_to_array(const class_instance<T> &c);

class InstanceToArrayVisitor {
public:
  array<var> flush_result() {
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
  void process_impl(const char *field_name, const OrFalse<T> &value) {
    if (value.bool_value) {
      process_impl(field_name, value.val());
    } else {
      add_value(field_name, var{false});
    }
  }

  template<class T>
  void process_impl(const char *field_name, const array<T> &value) {
    array<var> converted_value(value.size());
    for (auto it = value.begin(); it != value.end(); ++it) {
      process_impl("", it.get_value());
      converted_value.set_value(it.get_key(), result_.pop());
    }

    add_value(field_name, converted_value);
  }

  template<class I>
  void process_impl(const char *field_name, const class_instance<I> &instance) {
    add_value(field_name, instance.is_null() ? var{} : f$instance_to_array(instance));
  }

  template<class ...Args>
  void process_impl(const char *field_name, const std::tuple<Args...> &value) {
    InstanceToArrayVisitor tuple_processor;
    tuple_processor.result_.reserve(sizeof...(Args), 0, true);

    process_tuple(value, tuple_processor);
    add_value(field_name, tuple_processor.flush_result());
  }

  template<size_t Index = 0, class ...Args>
  std::enable_if_t<Index != sizeof...(Args)> process_tuple(const std::tuple<Args...> &value, InstanceToArrayVisitor &tuple_visitor) {
    tuple_visitor.process_impl("", std::get<Index>(value));
    process_tuple<Index + 1>(value, tuple_visitor);
  }

  template<size_t Index = 0, class ...Args>
  std::enable_if_t<Index == sizeof...(Args)> process_tuple(const std::tuple<Args...> &, InstanceToArrayVisitor &/*tuple_processor*/) {
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
  array<var> result_;
};

template<class T>
array<var> f$instance_to_array(const class_instance<T> &c) {
  if (c.is_null()) {
    return {};
  }
  InstanceToArrayVisitor visitor;
  c.get()->accept(visitor);
  return visitor.flush_result();
}
