// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/core-types/decl/optional.h"
#include "runtime-common/core/runtime-core.h"

bool f$rpc_parse(const string& new_rpc_data);

bool f$rpc_parse(const mixed& new_rpc_data);

bool f$rpc_parse(bool new_rpc_data);

bool f$rpc_parse(const Optional<string>& new_rpc_data);
