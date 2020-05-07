#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/function-pass.h"
#include "compiler/threading/data-stream.h"

/*
 * Пайп, который после inferring'а бежит по всем классам и что-то проверяет.
 */
class CheckClassesPass : public FunctionPassBase {
  constexpr static int32_t max_serialization_tag_value = std::numeric_limits<int8_t>::max();
  using used_serialization_tags_t = std::array<bool, max_serialization_tag_value>;

  static void analyze_class(ClassPtr klass);

  static void check_instance_fields_inited(ClassPtr klass);

  static void check_static_fields_inited(ClassPtr klass);

  static void check_serialized_fields(ClassPtr klass);

  static void fill_reserved_serialization_tags(used_serialization_tags_t &used_serialization_tags_for_fields, ClassPtr klass);

public:
  std::string get_description() override {
    return "Check classes";
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);

  bool on_start(FunctionPtr function);
};
