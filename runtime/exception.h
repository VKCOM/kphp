#pragma once

#include "runtime/kphp_core.h"

array<array<string>> f$debug_backtrace();


struct C$Exception {
  int ref_cnt = 0;
  string message;
  int code = 0;
  string file;
  int line = 0;
  array<array<string>> trace;
};

using Exception = class_instance<C$Exception>;

extern Exception CurException;

#define THROW_EXCEPTION(e) {Exception x_tmp___ = e; php_assert(CurException.is_null()); CurException = std::move(x_tmp___);}

#define CHECK_EXCEPTION(action) if (!CurException.is_null()) {action;}

#define TRY_CALL_(CallT, call, action) ({CallT x_tmp___ = (call); CHECK_EXCEPTION(action); std::move(x_tmp___);})
#define TRY_CALL_VOID_(call, action) ({(call); CHECK_EXCEPTION(action); void();})

#define TRY_CALL(CallT, ResT, call) TRY_CALL_(CallT, call, return (ResT()))
#define TRY_CALL_VOID(ResT, call) TRY_CALL_VOID_(call, return (ResT()))

#define TRY_CALL_EXIT(CallT, message, call) TRY_CALL_(CallT, call, php_critical_error (message))
#define TRY_CALL_VOID_EXIT(message, call) TRY_CALL_VOID_(call, php_critical_error (message))

Exception f$Exception$$__construct(const string &file, int line, const string &message = string(), int code = 0);

Exception f$err(const string &file, int line, const string &code, const string &desc = string());


string f$Exception$$getMessage(const Exception &e);

int f$Exception$$getCode(const Exception &e);

string f$Exception$$getFile(const Exception &e);

int f$Exception$$getLine(const Exception &e);

array<array<string>> f$Exception$$getTrace(const Exception &e);

string f$Exception$$getTraceAsString(const Exception &e);


void free_exception_lib();
