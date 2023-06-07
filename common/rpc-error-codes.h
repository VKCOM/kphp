// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

/**
 * Error codes
 *
 * Ranges:
 *   Query syntax errors                                                    -1000...-1999
 *   General semantic error in query, can't be executed                     -2000...-2999
 *   Error while processing query                                           -3000...-3999
 *   Different generic errors                                               -4000...-4999
 *   Application layer errors, normal to happen (like something not found)  -5000...-5999
 */

#define TL_ERROR_SYNTAX                      (-1000)
#define TL_ERROR_EXTRA_DATA                  (-1001)
#define TL_ERROR_HEADER                      (-1002)
#define TL_ERROR_WRONG_QUERY_ID              (-1003)

#define TL_ERROR_UNKNOWN_FUNCTION_ID         (-2000)
#define TL_ERROR_PROXY_NO_TARGET             (-2001)
#define TL_ERROR_WRONG_ACTOR_ID              (-2002)
#define TL_ERROR_TOO_LONG_STRING             (-2003)
#define TL_ERROR_VALUE_NOT_IN_RANGE          (-2004)
#define TL_ERROR_QUERY_INCORRECT             (-2005)
#define TL_ERROR_BAD_VALUE                   (-2006)
#define TL_ERROR_BINLOG_DISABLED             (-2007)
#define TL_ERROR_FEATURE_DISABLED            (-2008)
#define TL_ERROR_QUERY_IS_EMPTY              (-2009)
#define TL_ERROR_INVALID_CONNECTION_ID       (-2010)

#define TL_ERROR_QUERY_TIMEOUT               (-3000)
#define TL_ERROR_PROXY_INVALID_RESPONSE      (-3001)
#define TL_ERROR_NO_CONNECTIONS              (-3002)
#define TL_ERROR_INTERNAL                    (-3003)
#define TL_ERROR_AIO_FAIL                    (-3004)
#define TL_ERROR_AIO_TIMEOUT                 (-3005)
#define TL_ERROR_BINLOG_WAIT_TIMEOUT         (-3006)
#define TL_ERROR_AIO_MAX_RETRY_EXCEEDED      (-3007)
#define TL_ERROR_TTL                         (-3008)
#define TL_ERROR_OUT_OF_MEMORY               (-3009)
#define TL_ERROR_BAD_METAFILE                (-3010)
#define TL_ERROR_RESULT_TOO_LARGE            (-3011)
#define TL_ERROR_TOO_MANY_BAD_RESPONSES      (-3012)
#define TL_ERROR_FLOOD_CONTROL               (-3013)

#define TL_ERROR_UNKNOWN                         (-4000)
#define TL_ERROR_RESPONSE_SYNTAX                 (-4101)
#define TL_ERROR_RESPONSE_NOT_FOUND              (-4102)
#define TL_ERROR_TIMEOUT_IN_RPC_CLIENT           (-4103)
#define TL_ERROR_NO_CONNECTIONS_IN_RPC_CLIENT    (-4104)

#define TL_IS_USER_ERROR(x)                  ((x) <= -1000 && (x) > -3000)


