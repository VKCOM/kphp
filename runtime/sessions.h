#pragma once

#include "runtime/kphp_core.h"
#include "runtime/interface.h"

bool f$session_start(const array<mixed> &options = array<mixed>());
bool f$session_abort();