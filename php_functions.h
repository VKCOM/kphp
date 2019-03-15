// use #ifndef guard instead of #pragma once,
// because this header is used as precompiled,
// and gcc fires the warning: #pragma once in main file
#ifndef PHP_FUNCTONS_H
#define PHP_FUNCTONS_H

#include "runtime/array_functions.h"
#include "runtime/bcmath.h"
#include "runtime/curl.h"
#include "runtime/datetime.h"
#include "runtime/drivers.h"
#include "runtime/exception.h"
#include "runtime/files.h"
#include "runtime/instance_cache.h"
#include "runtime/integer_types.h"
#include "runtime/interface.h"
#include "runtime/mail.h"
#include "runtime/math_functions.h"
#include "runtime/mbstring.h"
#include "runtime/misc.h"
#include "runtime/net_events.h"
#include "runtime/openssl.h"
#include "runtime/regexp.h"
#include "runtime/resumable.h"
#include "runtime/rpc.h"
#include "runtime/streams.h"
#include "runtime/string_functions.h"
#include "runtime/url.h"
#include "runtime/vkext.h"
#include "runtime/vkext_stats.h"
#include "runtime/zlib.h"
#include "runtime/refcountable_php_classes.h"
#include "PHP/tl/tl_init.h"

#endif // PHP_FUNCTIONS_H
