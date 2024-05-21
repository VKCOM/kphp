#include "kphp-core/kphp_core.h"
#include "runtime/msgpack-serialization.h"
#include <array>
#include <gtest/gtest.h>
#include <utility>

constexpr auto edge_int_values = std::array{
  0_i64,
  1_i64,
  -1_i64,
  std::numeric_limits<int64_t>::max(),
  std::numeric_limits<int64_t>::min(),
  static_cast<int64_t>(std::numeric_limits<int32_t>::max()),
  static_cast<int64_t>(std::numeric_limits<int32_t>::min()),
  std::numeric_limits<int32_t>::max() + 1_i64,
  std::numeric_limits<int32_t>::min() + 1_i64,
  std::numeric_limits<int32_t>::max() - 1_i64,
  std::numeric_limits<int32_t>::min() - 1_i64,
  static_cast<int64_t>(std::numeric_limits<int16_t>::max()),
  static_cast<int64_t>(std::numeric_limits<int16_t>::min()),
  std::numeric_limits<int16_t>::max() + 1_i64,
  std::numeric_limits<int16_t>::min() + 1_i64,
  std::numeric_limits<int16_t>::max() - 1_i64,
  std::numeric_limits<int16_t>::min() - 1_i64,
  static_cast<int64_t>(std::numeric_limits<int16_t>::max()),
  static_cast<int64_t>(std::numeric_limits<int16_t>::min()),
  std::numeric_limits<int8_t>::max() + 1_i64,
  std::numeric_limits<int8_t>::min() + 1_i64,
  std::numeric_limits<int8_t>::max() - 1_i64,
  std::numeric_limits<int8_t>::min() - 1_i64,
};
constexpr auto edge_fp_values = std::array{+0.0, -0.0, 1.1e-300, -1.1e-300, 1.0, -1.0, std::numeric_limits<double>::min(), std::numeric_limits<double>::max()};

const auto edge_string_values = std::array{
  string(""),
  string("0"),
  string("a"),
  string(" "),
  string("   "),
  string("\n"),
  string("\r\n"),
  string("\t"),
  string("\a"),
  string("\b"),
  string("42num"),
  string("num42"),
  string("multi word"),
  string("multi 12 word 23 with digits 42"),
  string("! @ #  % ^ & * ()-_+="),
  string("! @ #  % ^\0& * ()-_+="),
};

template<typename T>
void assert_single(const T data) {
  Optional<string> serialized = f$msgpack_serialize(data);
  ASSERT_TRUE(eq2(data, f$msgpack_deserialize<T>(serialized.val())));
}

TEST(msgpack, just_int) {
  assert_single<int64_t>(42);
}

TEST(msgpack, just_double) {
  assert_single<double>(3.1415);
}

TEST(msgpack, just_string) {
  assert_single<string>(string("just a string"));
}

TEST(msgpack, just_array) {
  assert_single<array<int64_t>>(array<int64_t>::create(1, 2, 3, 4, 5, 6));
  assert_single<array<double>>(array<double>::create());
}

TEST(msgpack, primitives) {
  for (const auto val : edge_int_values) {
    assert_single<int64_t>(val);
  }

  for (const auto val : edge_fp_values) {
    assert_single<double>(val);
  }

  for (const auto &val : edge_string_values) {
    assert_single<string>(val);
  }
}

TEST(msgpack, mixed) {
  std::vector<mixed> vec;
  vec.reserve(std::size(edge_int_values) + std::size(edge_fp_values) + std::size(edge_string_values));
  for (auto val : edge_int_values) {
    vec.emplace_back(val);
  }
  for (auto val : edge_fp_values) {
    vec.emplace_back(val);
  }
  for (const auto &val : edge_string_values) {
    vec.emplace_back(val);
  }

  vec.emplace_back(array<double>::create(0.0, -1.0, -1e20));
  vec.emplace_back(array<double>::create());
  vec.emplace_back(array<string>::create(string("42"), string("\r\n42!@#%^&*()_+")));
  vec.emplace_back(array<string>::create());
  vec.emplace_back(array<mixed>::create(1, 2.0, string("\r\n42!@#%^&*()_+")));

  for (auto &&val : vec) {
    assert_single<mixed>(val);
  }
}

struct Stub : public refcountable_php_classes<Stub> {
  mixed m{};
  double d{};
  int64_t i{};
  string s{};
  array<mixed> a{};

  Stub() = default;
  Stub(mixed m_, double d_, int64_t i_, string s_, array<mixed> a_)
    : m(std::move(m_))
    , d(d_)
    , i(i_)
    , s(std::move(s_))
    , a(std::move(a_)) {}

  void msgpack_pack(vk::msgpack::packer<string_buffer> &packer) const noexcept {
    packer.pack_array(10);
    packer.pack(1);
    vk::msgpack::packer_float32_decorator::pack_value(packer, d);
    packer.pack(2);
    vk::msgpack::packer_float32_decorator::pack_value(packer, i);
    packer.pack(3);
    vk::msgpack::packer_float32_decorator::pack_value(packer, s);
    packer.pack(4);
    vk::msgpack::packer_float32_decorator::pack_value(packer, m);
    packer.pack(5);
    vk::msgpack::packer_float32_decorator::pack_value(packer, a);
  }
  void msgpack_unpack(const vk::msgpack::object &msgpack_o) {
    if (msgpack_o.type != vk::msgpack::stored_type::ARRAY) {
      throw vk::msgpack::type_error{};
    }
    auto arr = msgpack_o.via.array;
    for (size_t counter = 0; counter < arr.size; counter += 2) {
      auto tag = arr.ptr[counter].as<uint8_t>();
      auto elem = arr.ptr[counter + 1];
      switch (tag) {
        case 1:
          elem.convert(d);
          break;
        case 2:
          elem.convert(i);
          break;
        case 3:
          elem.convert(s);
          break;
        case 4:
          elem.convert(m);
          break;
        case 5:
          elem.convert(a);
          break;
        default:
          break;
      }
    }
  }
};

bool eq2(const Stub &lhs, const Stub &rhs) {
  return eq2(lhs.i, rhs.i) && eq2(lhs.d, rhs.d) && eq2(lhs.s, rhs.s) && eq2(lhs.m, rhs.m) && eq2(lhs.a, rhs.a);
}

TEST(mspack, custom_type) {
  assert_single<Stub>({});
  assert_single<Stub>(Stub(42_i64, -0.0, 42_i64, string("string"), array<mixed>::create(string("0"), string(""), 42, -0.0)));
  assert_single<Stub>(Stub(42_i64, -0.0, 42_i64, string("string"), {}));
  assert_single<Stub>(Stub(42_i64, -0.0, 42_i64, {}, array<mixed>::create(string("0"), string(""), 42, -0.0)));
  assert_single<Stub>(Stub({}, -0.0, 42_i64, string("string"), array<mixed>::create(string("0"), string(""), 42, -0.0)));
}

TEST(msgpack, array_extended) {
  assert_single<array<string>>(array<string>::create(string(""), string("0"), string("a"), string(" "), string("   "), string("\n"), string("\r\n"),
                                                     string("\t"), string("\a"), string("\b"), string("42num"), string("num42"), string("multi word"),
                                                     string("multi 12 word 23 with digits 42"), string("! @ #  % ^ & * ()-_+=")));

  assert_single<array<array<double>>>(array<array<double>>::create(array<double>::create(), array<double>::create(edge_fp_values[0]),
                                                                   array<double>::create(edge_fp_values[1], edge_fp_values[2], edge_fp_values[3]),
                                                                   array<double>::create()));

  assert_single<array<mixed>>(array<mixed>::create(1, -0.0, string("! @ #  % ^\0& * ()-_+="),
                                                   array<int64_t>::create(1, 2, 3, 4, std::numeric_limits<int64_t>::max()),
                                                   array<string>::create(string("qwerty"))));
  assert_single<array<Stub>>(array<Stub>::create(Stub(42_i64, -0.0, 42_i64, string("string"), array<mixed>::create(string("0"), string(""), 42, -0.0)),
                                                 Stub(42_i64, -0.0, 42_i64, string("string"), {}),
                                                 Stub(42_i64, -0.0, 42_i64, {}, array<mixed>::create(string("0"), string(""), 42, -0.0)),
                                                 Stub({}, -0.0, 42_i64, string("string"), array<mixed>::create(string("0"), string(""), 42, -0.0))));
}
