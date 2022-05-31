//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2014 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "runtime/msgpack/adaptor/adaptor_base.h"
#include "runtime/msgpack/object_fwd.h"
#include "runtime/msgpack/zone.h"

#include <cstring>
#include <cassert>
#include <stdexcept>
#include <typeinfo>
#include <limits>
#include <ostream>
#include <typeinfo>
#include <iomanip>
#include <memory>

namespace msgpack {

struct object_kv {
    msgpack::object key;
    msgpack::object val;
};

namespace detail {
template <typename Stream, typename T>
struct packer_serializer {
    static msgpack::packer<Stream>& pack(msgpack::packer<Stream>& o, const T& v) {
        v.msgpack_pack(o);
        return o;
    }
};
} // namespace detail

// Adaptor functors' member functions definitions.
template <typename T, typename Enabler>
inline
msgpack::object const&
adaptor::convert<T, Enabler>::operator()(msgpack::object const& o, T& v) const {
    v.msgpack_unpack(o);
    return o;
}

template <typename T, typename Enabler>
template <typename Stream>
inline
msgpack::packer<Stream>&
adaptor::pack<T, Enabler>::operator()(msgpack::packer<Stream>& o, T const& v) const {
    return detail::packer_serializer<Stream, T>::pack(o, v);
}

// deconvert operator

template <typename Stream>
template <typename T>
inline msgpack::packer<Stream>& packer<Stream>::pack(const T& v) {
    return adaptor::pack<T>{}(*this, v);
}

template <typename T>
inline std::enable_if_t<!std::is_array_v<T> && !std::is_pointer_v<T>, T&>
object::convert(T& v) const {
  adaptor::convert<T>{}(*this, v);
  return v;
}

template <typename T>
inline T object::as() const {
    T v;
    convert(v);
    return v;
}

}  // namespace msgpack
