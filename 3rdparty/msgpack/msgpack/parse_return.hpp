//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2017 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

namespace msgpack {

// for internal use
enum parse_return {
    PARSE_SUCCESS              =  2,
    PARSE_EXTRA_BYTES          =  1,
    PARSE_CONTINUE             =  0,
    PARSE_PARSE_ERROR          = -1,
    PARSE_STOP_VISITOR         = -2
};

}  // namespace msgpack
