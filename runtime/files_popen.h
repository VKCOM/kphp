#pragma once

#include "runtime/kphp_core.h"

using Stream =mixed;

//2022-04-19 av (comm644)
Stream f$popen(const string &stream, const string &mode);
bool f$pclose(const Stream &stream);

void global_init_popen_lib();

void free_popen_lib();
