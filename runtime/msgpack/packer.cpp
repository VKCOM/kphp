// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/msgpack/packer.h"

#include "runtime-core/runtime-core.h"
#include "runtime/msgpack/sysdep.h"

namespace vk::msgpack {

template<typename T>
static char take8_8(T d) noexcept {
  return static_cast<char>(reinterpret_cast<uint8_t *>(&d)[0]);
}

template<typename T>
static char take8_16(T d) noexcept {
  return static_cast<char>(reinterpret_cast<uint8_t *>(&d)[0]);
}

template<typename T>
static char take8_32(T d) noexcept {
  return static_cast<char>(reinterpret_cast<uint8_t *>(&d)[0]);
}

template<typename T>
static char take8_64(T d) noexcept {
  return static_cast<char>(reinterpret_cast<uint8_t *>(&d)[0]);
}

template<typename Stream>
void packer<Stream>::pack_uint8(uint8_t d) noexcept {
  pack_imp_uint8(d);
}

template<typename Stream>
void packer<Stream>::pack_uint32(uint32_t d) noexcept {
  pack_imp_uint32(d);
}

template<typename Stream>
void packer<Stream>::pack_uint64(uint64_t d) noexcept {
  pack_imp_uint64(d);
}

template<typename Stream>
void packer<Stream>::pack_int32(int32_t d) noexcept {
  pack_imp_int32(d);
}

template<typename Stream>
void packer<Stream>::pack_int64(int64_t d) noexcept {
  pack_imp_int64(d);
}

template<typename Stream>
void packer<Stream>::pack_float(float d) noexcept {
  union {
    float f;
    uint32_t i;
  } mem;
  mem.f = d;
  char buf[5];
  buf[0] = static_cast<char>(0xcau);
  _msgpack_store32(&buf[1], mem.i);
  append_buffer(buf, 5);
}

template<typename Stream>
void packer<Stream>::pack_double(double d) noexcept {
  union {
    double f;
    uint64_t i;
  } mem;
  mem.f = d;
  char buf[9];
  buf[0] = static_cast<char>(0xcbu);

#if defined(TARGET_OS_IPHONE)
  // ok
#elif defined(__arm__) && !(__ARM_EABI__) // arm-oabi
  // https://github.com/msgpack/msgpack-perl/pull/1
  mem.i = (mem.i & 0xFFFFFFFFUL) << 32UL | (mem.i >> 32UL);
#endif
  _msgpack_store64(&buf[1], mem.i);
  append_buffer(buf, 9);
}

template<typename Stream>
void packer<Stream>::pack_nil() noexcept {
  const char d = static_cast<char>(0xc0u);
  append_buffer(&d, 1);
}

template<typename Stream>
void packer<Stream>::pack_true() noexcept {
  const char d = static_cast<char>(0xc3u);
  append_buffer(&d, 1);
}

template<typename Stream>
void packer<Stream>::pack_false() noexcept {
  const char d = static_cast<char>(0xc2u);
  append_buffer(&d, 1);
}

template<typename Stream>
void packer<Stream>::pack_array(uint32_t n) noexcept {
  if (n < 16) {
    char d = static_cast<char>(0x90u | n);
    append_buffer(&d, 1);
  } else if (n < 65536) {
    char buf[3];
    buf[0] = static_cast<char>(0xdcu);
    _msgpack_store16(&buf[1], static_cast<uint16_t>(n));
    append_buffer(buf, 3);
  } else {
    char buf[5];
    buf[0] = static_cast<char>(0xddu);
    _msgpack_store32(&buf[1], static_cast<uint32_t>(n));
    append_buffer(buf, 5);
  }
}

template<typename Stream>
void packer<Stream>::pack_map(uint32_t n) noexcept {
  if (n < 16) {
    unsigned char d = static_cast<unsigned char>(0x80u | n);
    char buf = take8_8(d);
    append_buffer(&buf, 1);
  } else if (n < 65536) {
    char buf[3];
    buf[0] = static_cast<char>(0xdeu);
    _msgpack_store16(&buf[1], static_cast<uint16_t>(n));
    append_buffer(buf, 3);
  } else {
    char buf[5];
    buf[0] = static_cast<char>(0xdfu);
    _msgpack_store32(&buf[1], static_cast<uint32_t>(n));
    append_buffer(buf, 5);
  }
}

template<typename Stream>
void packer<Stream>::pack_str(uint32_t l) noexcept {
  if (l < 32) {
    unsigned char d = static_cast<uint8_t>(0xa0u | l);
    char buf = take8_8(d);
    append_buffer(&buf, 1);
  } else if (l < 256) {
    char buf[2];
    buf[0] = static_cast<char>(0xd9u);
    buf[1] = static_cast<uint8_t>(l);
    append_buffer(buf, 2);
  } else if (l < 65536) {
    char buf[3];
    buf[0] = static_cast<char>(0xdau);
    _msgpack_store16(&buf[1], static_cast<uint16_t>(l));
    append_buffer(buf, 3);
  } else {
    char buf[5];
    buf[0] = static_cast<char>(0xdbu);
    _msgpack_store32(&buf[1], static_cast<uint32_t>(l));
    append_buffer(buf, 5);
  }
}

template<typename Stream>
void packer<Stream>::pack_str_body(const char *b, uint32_t l) noexcept {
  append_buffer(b, l);
}

template<typename Stream>
template<typename T>
void packer<Stream>::pack_imp_uint8(T d) noexcept {
  if (d < (1 << 7)) {
    /* fixnum */
    char buf = take8_8(d);
    append_buffer(&buf, 1);
  } else {
    /* unsigned 8 */
    char buf[2] = {static_cast<char>(0xccu), take8_8(d)};
    append_buffer(buf, 2);
  }
}

template<typename Stream>
template<typename T>
void packer<Stream>::pack_imp_uint32(T d) noexcept {
  if (d < (1 << 8)) {
    if (d < (1 << 7)) {
      /* fixnum */
      char buf = take8_32(d);
      append_buffer(&buf, 1);
    } else {
      /* unsigned 8 */
      char buf[2] = {static_cast<char>(0xccu), take8_32(d)};
      append_buffer(buf, 2);
    }
  } else {
    if (d < (1 << 16)) {
      /* unsigned 16 */
      char buf[3];
      buf[0] = static_cast<char>(0xcdu);
      _msgpack_store16(&buf[1], static_cast<uint16_t>(d));
      append_buffer(buf, 3);
    } else {
      /* unsigned 32 */
      char buf[5];
      buf[0] = static_cast<char>(0xceu);
      _msgpack_store32(&buf[1], static_cast<uint32_t>(d));
      append_buffer(buf, 5);
    }
  }
}

template<typename Stream>
template<typename T>
void packer<Stream>::pack_imp_uint64(T d) noexcept {
  if (d < (1ULL << 8)) {
    if (d < (1ULL << 7)) {
      /* fixnum */
      char buf = take8_64(d);
      append_buffer(&buf, 1);
    } else {
      /* unsigned 8 */
      char buf[2] = {static_cast<char>(0xccu), take8_64(d)};
      append_buffer(buf, 2);
    }
  } else {
    if (d < (1ULL << 16)) {
      /* unsigned 16 */
      char buf[3];
      buf[0] = static_cast<char>(0xcdu);
      _msgpack_store16(&buf[1], static_cast<uint16_t>(d));
      append_buffer(buf, 3);
    } else if (d < (1ULL << 32)) {
      /* unsigned 32 */
      char buf[5];
      buf[0] = static_cast<char>(0xceu);
      _msgpack_store32(&buf[1], static_cast<uint32_t>(d));
      append_buffer(buf, 5);
    } else {
      /* unsigned 64 */
      char buf[9];
      buf[0] = static_cast<char>(0xcfu);
      _msgpack_store64(&buf[1], d);
      append_buffer(buf, 9);
    }
  }
}

template<typename Stream>
template<typename T>
void packer<Stream>::pack_imp_int32(T d) noexcept {
  if (d < -(1 << 5)) {
    if (d < -(1 << 15)) {
      /* signed 32 */
      char buf[5];
      buf[0] = static_cast<char>(0xd2u);
      _msgpack_store32(&buf[1], static_cast<int32_t>(d));
      append_buffer(buf, 5);
    } else if (d < -(1 << 7)) {
      /* signed 16 */
      char buf[3];
      buf[0] = static_cast<char>(0xd1u);
      _msgpack_store16(&buf[1], static_cast<int16_t>(d));
      append_buffer(buf, 3);
    } else {
      /* signed 8 */
      char buf[2] = {static_cast<char>(0xd0u), take8_32(d)};
      append_buffer(buf, 2);
    }
  } else if (d < (1 << 7)) {
    /* fixnum */
    char buf = take8_32(d);
    append_buffer(&buf, 1);
  } else {
    if (d < (1 << 8)) {
      /* unsigned 8 */
      char buf[2] = {static_cast<char>(0xccu), take8_32(d)};
      append_buffer(buf, 2);
    } else if (d < (1 << 16)) {
      /* unsigned 16 */
      char buf[3];
      buf[0] = static_cast<char>(0xcdu);
      _msgpack_store16(&buf[1], static_cast<uint16_t>(d));
      append_buffer(buf, 3);
    } else {
      /* unsigned 32 */
      char buf[5];
      buf[0] = static_cast<char>(0xceu);
      _msgpack_store32(&buf[1], static_cast<uint32_t>(d));
      append_buffer(buf, 5);
    }
  }
}

template<typename Stream>
template<typename T>
void packer<Stream>::pack_imp_int64(T d) noexcept {
  if (d < -(1LL << 5)) {
    if (d < -(1LL << 15)) {
      if (d < -(1LL << 31)) {
        /* signed 64 */
        char buf[9];
        buf[0] = static_cast<char>(0xd3u);
        _msgpack_store64(&buf[1], d);
        append_buffer(buf, 9);
      } else {
        /* signed 32 */
        char buf[5];
        buf[0] = static_cast<char>(0xd2u);
        _msgpack_store32(&buf[1], static_cast<int32_t>(d));
        append_buffer(buf, 5);
      }
    } else {
      if (d < -(1 << 7)) {
        /* signed 16 */
        char buf[3];
        buf[0] = static_cast<char>(0xd1u);
        _msgpack_store16(&buf[1], static_cast<int16_t>(d));
        append_buffer(buf, 3);
      } else {
        /* signed 8 */
        char buf[2] = {static_cast<char>(0xd0u), take8_64(d)};
        append_buffer(buf, 2);
      }
    }
  } else if (d < (1 << 7)) {
    /* fixnum */
    char buf = take8_64(d);
    append_buffer(&buf, 1);
  } else {
    if (d < (1LL << 16)) {
      if (d < (1 << 8)) {
        /* unsigned 8 */
        char buf[2] = {static_cast<char>(0xccu), take8_64(d)};
        append_buffer(buf, 2);
      } else {
        /* unsigned 16 */
        char buf[3];
        buf[0] = static_cast<char>(0xcdu);
        _msgpack_store16(&buf[1], static_cast<uint16_t>(d));
        append_buffer(buf, 3);
      }
    } else {
      if (d < (1LL << 32)) {
        /* unsigned 32 */
        char buf[5];
        buf[0] = static_cast<char>(0xceu);
        _msgpack_store32(&buf[1], static_cast<uint32_t>(d));
        append_buffer(buf, 5);
      } else {
        /* unsigned 64 */
        char buf[9];
        buf[0] = static_cast<char>(0xcfu);
        _msgpack_store64(&buf[1], d);
        append_buffer(buf, 9);
      }
    }
  }
}

template<typename Stream>
void packer<Stream>::append_buffer(const char *buf, size_t len) noexcept {
  static_assert(noexcept(std::declval<Stream>().write(buf, len)), "require Stream::write() to be noxecept");
  stream_.write(buf, len);
}

template class packer<string_buffer>;

uint32_t packer_float32_decorator::serialize_as_float32_ = 0;

} // namespace vk::msgpack
