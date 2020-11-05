// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VKEXT_TL_MEMCACHE_H__
#define __VKEXT_TL_MEMCACHE_H__

void rpc_memcache_connect(INTERNAL_FUNCTION_PARAMETERS);
void rpc_memcache_delete(INTERNAL_FUNCTION_PARAMETERS);
void rpc_memcache_incr(int op, INTERNAL_FUNCTION_PARAMETERS);
void rpc_memcache_store(int op, INTERNAL_FUNCTION_PARAMETERS);
void rpc_memcache_get(INTERNAL_FUNCTION_PARAMETERS);
void php_rpc_fetch_memcache_value(INTERNAL_FUNCTION_PARAMETERS);
#endif
