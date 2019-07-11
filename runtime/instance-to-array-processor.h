#pragma once

#include "runtime/instance-visitor.h"
#include "runtime/kphp_core.h"

class InstanceToArrayProcessor {
public:
  array<var> flush_result() {
    return std::move(result);
  }

  template<class T>
  bool process(const char *field_name, const T &value) {
    process_impl(field_name, value);
    return true;
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
      converted_value.set_value(it.get_key(), result.pop());
    }

    add_value(field_name, converted_value);
  }

  template<class I>
  void process_impl(const char *field_name, const class_instance<I> &instance) {
    static_assert(!std::is_abstract<I>::value, "you may not convert interface to array");
    if (instance.is_null()) {
      add_value(field_name, var{});
      return;
    }

    InstanceToArrayProcessor processor;
    InstanceVisitor<I, InstanceToArrayProcessor> visitor{instance, processor};
    instance->accept(visitor);

    add_value(field_name, processor.flush_result());
  }

  template<class ...Args>
  void process_impl(const char *field_name, const std::tuple<Args...> &value) {
    InstanceToArrayProcessor tuple_processor;
    tuple_processor.result.reserve(sizeof...(Args), 0, true);

    process_tuple(value, tuple_processor);
    add_value(field_name, tuple_processor.flush_result());
  }

  template<size_t Index = 0, class ...Args>
  vk::enable_if_t<Index != sizeof...(Args)> process_tuple(const std::tuple<Args...> &value, InstanceToArrayProcessor &tuple_processor) {
    tuple_processor.process_impl("", std::get<Index>(value));
    process_tuple<Index + 1>(value, tuple_processor);
  }

  template<size_t Index = 0, class ...Args>
  vk::enable_if_t<Index == sizeof...(Args)> process_tuple(const std::tuple<Args...> &, InstanceToArrayProcessor &/*tuple_processor*/) {
  }

  template<class T>
  void add_value(const char *field_name, T &&value) {
    if (field_name[0] != '\0') {
      result.set_value(string{field_name}, std::forward<T>(value));
    } else {
      result.push_back(std::forward<T>(value));
    }
  }

private:
  array<var> result;
};

template<class T>
array<var> f$instance_to_array(const class_instance<T> &c) {
  InstanceToArrayProcessor processor;
  c->accept(InstanceVisitor<T, InstanceToArrayProcessor>{c, processor});
  return processor.flush_result();
}
