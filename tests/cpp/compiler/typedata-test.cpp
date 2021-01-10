#include <gtest/gtest.h>

#include "compiler/inferring/type-data.h"

TEST(typedata_test, typedata_human_readable) {
  auto *td_int = TypeData::get_type(tp_int);
  ASSERT_EQ(td_int->as_human_readable(false), "int");

  auto *td_null = TypeData::get_type(tp_any)->clone();
  td_null->set_lca(tp_Null);
  ASSERT_EQ(td_null->as_human_readable(false), "null");

  auto *td_arr_float = TypeData::get_type(tp_array, tp_float);
  ASSERT_EQ(td_arr_float->as_human_readable(false), "float[]");

  auto *td_arr_any = TypeData::get_type(tp_array, tp_any);
  ASSERT_EQ(td_arr_any->as_human_readable(false), "array");

  auto *td_float_or_null = TypeData::get_type(tp_float)->clone();
  td_float_or_null->set_lca(tp_Null);
  ASSERT_EQ(td_float_or_null->as_human_readable(false), "?float");

  auto *td_string_or_null_or_false = TypeData::get_type(tp_string)->clone();
  td_string_or_null_or_false->set_lca(tp_Null);
  td_string_or_null_or_false->set_lca(tp_False);
  ASSERT_EQ(td_string_or_null_or_false->as_human_readable(false), "string|false|null");

  auto *td_arr_arr_float_or_null = TypeData::create_array_of(TypeData::create_array_of(td_float_or_null));
  ASSERT_EQ(td_arr_arr_float_or_null->as_human_readable(false), "(?float)[][]");

  auto *td_arr_optional_string_or_false = TypeData::create_array_of(td_string_or_null_or_false)->clone();
  td_arr_optional_string_or_false->set_lca(tp_False);
  ASSERT_EQ(td_arr_optional_string_or_false->as_human_readable(false), "(string|false|null)[]|false");

  auto *td_future_bool = TypeData::get_type(tp_future)->clone();
  td_future_bool->set_lca_at(MultiKey::any_key(1), TypeData::get_type(tp_bool));
  ASSERT_EQ(td_future_bool->as_human_readable(false), "future<bool>");

  auto *td_tuple = TypeData::get_type(tp_tuple)->clone();
  td_tuple->set_lca_at(MultiKey({Key::int_key(0)}), td_arr_float);
  td_tuple->set_lca_at(MultiKey({Key::int_key(1)}), TypeData::get_type(tp_any));
  td_tuple->set_lca_at(MultiKey({Key::int_key(2)}), TypeData::get_type(tp_future_queue));
  ASSERT_EQ(td_tuple->as_human_readable(false), "tuple(float[], any, future_queue<any>)");

  auto *td_shape = TypeData::get_type(tp_shape)->clone();
  td_shape->set_lca_at(MultiKey({Key::string_key("x")}), TypeData::get_type(tp_int));
  td_shape->set_lca_at(MultiKey({Key::string_key("y")}), TypeData::get_type(tp_array, tp_False));
  ASSERT_EQ(td_shape->as_human_readable(false), "shape(x:int, y:false[])");
}
