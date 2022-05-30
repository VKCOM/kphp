//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2017 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_PARSE_RETURN_HPP
#define MSGPACK_PARSE_RETURN_HPP

#include "msgpack/versioning.hpp"

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
/// @endcond


// for internal use
typedef enum {
    PARSE_SUCCESS              =  2,
    PARSE_EXTRA_BYTES          =  1,
    PARSE_CONTINUE             =  0,
    PARSE_PARSE_ERROR          = -1
} parse_return;

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v2) {
  /// @endcond


  // for internal use
  typedef enum {
    PARSE_SUCCESS              = v1::PARSE_SUCCESS,
    PARSE_EXTRA_BYTES          = v1::PARSE_EXTRA_BYTES,
    PARSE_CONTINUE             = v1::PARSE_CONTINUE,
    PARSE_PARSE_ERROR          = v1::PARSE_PARSE_ERROR,
    PARSE_STOP_VISITOR         = -2
  } parse_return;

  /// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v2)
/// @endcond

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v3) {
/// @endcond

using v2::parse_return;

using v2::PARSE_SUCCESS;
using v2::PARSE_EXTRA_BYTES;
using v2::PARSE_CONTINUE;
using v2::PARSE_PARSE_ERROR;
using v2::PARSE_STOP_VISITOR;

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v3)
/// @endcond

}  // namespace msgpack


#endif // MSGPACK_PARSE_RETURN_HPP
