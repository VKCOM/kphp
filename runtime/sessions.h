#pragma once

#include "runtime-core/runtime-core.h"

bool f$session_start(const array<mixed> &options = array<mixed>());
bool f$session_abort();
bool f$session_commit();
bool f$session_write_close();
Optional<int64_t> f$session_gc();
int64_t f$session_status();
Optional<string> f$session_encode();
bool f$session_decode(const string &data);
array<mixed> f$session_get_cookie_params();
Optional<string> f$session_id(const Optional<string> &id = Optional<string>());
bool f$session_reset();
Optional<string> f$session_save_path(const Optional<string> &path = Optional<string>());
bool f$session_unset();

// TO-DO:
// bool f$session_destroy();
// bool f$set_cookie_params();
// bool f$session_set_cookie_params();
// void f$session_register_shutdown();
// bool f$session_regenerate_id();
// bool f$session_create_id();