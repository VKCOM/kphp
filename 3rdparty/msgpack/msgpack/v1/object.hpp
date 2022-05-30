//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2014 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V1_OBJECT_HPP
#define MSGPACK_V1_OBJECT_HPP

#include "msgpack/adaptor/adaptor_base.hpp"
#include "msgpack/v1/object_fwd.hpp"

#include <cstring>
#include <cassert>
#include <stdexcept>
#include <typeinfo>
#include <limits>
#include <ostream>
#include <typeinfo>
#include <iomanip>

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v3) {
/// @endcond

struct object_kv {
    msgpack::object key;
    msgpack::object val;
};

/// The class holds object and zone
class object_handle {
public:
    /// Constructor that creates nil object and null zone.
    object_handle() {}

    /// Constructor that creates an object_handle holding object `obj` and zone `z`.
    /**
     * @param obj object
     * @param z zone
     */
    object_handle(
        msgpack::object const& obj,
        msgpack::unique_ptr<msgpack::zone>&& z
    ) :
        m_obj(obj), m_zone(msgpack::move(z)) { }

    void set(msgpack::object const& obj)
        { m_obj = obj; }

    /// Get object reference
    /**
     * @return object
     */
    const msgpack::object& get() const
        { return m_obj; }

    /**
     * @return object (to mimic smart pointers).
     */
    const msgpack::object& operator*() const
        { return get(); }

    /**
     * @return the address of the object (to mimic smart pointers).
     */
    const msgpack::object* operator->() const
        { return &get(); }

    /// Get unique_ptr reference of zone.
    /**
     * @return unique_ptr reference of zone
     */
    msgpack::unique_ptr<msgpack::zone>& zone()
        { return m_zone; }

    /// Get unique_ptr const reference of zone.
    /**
     * @return unique_ptr const reference of zone
     */
    const msgpack::unique_ptr<msgpack::zone>& zone() const
        { return m_zone; }

private:
    msgpack::object m_obj;
    msgpack::unique_ptr<msgpack::zone> m_zone;
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
inline msgpack::packer<Stream>& packer<Stream>::pack(const T& v)
{
    msgpack::operator<<(*this, v);
    return *this;
}

template <typename T>
inline
typename msgpack::enable_if<
    !msgpack::is_array<T>::value && !msgpack::is_pointer<T>::value,
    T&
>::type
object::convert(T& v) const
{
    msgpack::operator>>(*this, v);
    return v;
}

template <typename T>
inline T object::as() const {
    T v;
    convert(v);
    return v;
}

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

}  // namespace msgpack

#endif // MSGPACK_V1_OBJECT_HPP
