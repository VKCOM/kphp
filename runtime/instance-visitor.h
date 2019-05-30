#pragma once

#include "runtime/kphp_core.h"
#include "common/type_traits/traits.h"

template<typename I, typename ProcessorImpl>
class InstanceVisitor {
public:
  explicit InstanceVisitor(const class_instance<I> &instance, ProcessorImpl &processor) :
    instance_(instance),
    processor_(processor) {
  }

  template<typename T>
  void operator()(const char *field_name, T I::* value) {
    // we need to process each element
    if (!processor_.process(field_name, instance_.get()->*value)) {
      is_ok_ = false;
    }
  }

  bool is_ok() const { return is_ok_; }

private:
  bool is_ok_{true};
  const class_instance<I> &instance_;
  ProcessorImpl &processor_;
};
