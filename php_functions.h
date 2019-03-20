// use #ifndef guard instead of #pragma once,
// because this header is used as precompiled,
// and gcc fires the warning: #pragma once in main file
#ifndef PHP_FUNCTONS_H
#define PHP_FUNCTONS_H

#include <utility>

#include "runtime/array_functions.h"
#include "runtime/bcmath.h"
#include "runtime/curl.h"
#include "runtime/datetime.h"
#include "runtime/exception.h"
#include "runtime/files.h"
#include "runtime/instance-to-array-processor.h"
#include "runtime/instance_cache.h"
#include "runtime/integer_types.h"
#include "runtime/interface.h"
#include "runtime/mail.h"
#include "runtime/math_functions.h"
#include "runtime/mbstring.h"
#include "runtime/memcache.h"
#include "runtime/memory_usage.h"
#include "runtime/misc.h"
#include "runtime/mysql.h"
#include "runtime/net_events.h"
#include "runtime/null_coalesce.h"
#include "runtime/on_kphp_warning_callback.h"
#include "runtime/openssl.h"
#include "runtime/refcountable_php_classes.h"
#include "runtime/regexp.h"
#include "runtime/resumable.h"
#include "runtime/rpc.h"
#include "runtime/streams.h"
#include "runtime/string_functions.h"
#include "runtime/tl/rpc_function.h"
#include "runtime/tl/rpc_query.h"
#include "runtime/tl/rpc_request.h"
#include "runtime/tl/rpc_response.h"
#include "runtime/tl/tl_builtins.h"
#include "runtime/typed_rpc.h"
#include "runtime/url.h"
#include "runtime/vkext.h"
#include "runtime/vkext_stats.h"
#include "runtime/zlib.h"

#endif // PHP_FUNCTIONS_H
