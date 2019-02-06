#pragma once

#include "runtime/kphp_core.h"

array<array<string>> f$debug_backtrace();


class Exception {
public:
  bool bool_value;

  string message;
  int code;
  string file;
  int line;
  array<array<string>> trace;

  Exception();
  Exception(const string &file, int line, const string &message = string(), int code = 0);

  Exception &operator=(bool value);
  Exception(bool value);
};

extern Exception *CurException;

#define THROW_EXCEPTION(e) {Exception x_tmp___ = e; php_assert (!CurException); CurException = (Exception *)dl::allocate (sizeof (Exception)); new (CurException) Exception ((x_tmp___));}
#define TRY_CALL(CallT, ResT, call) ({CallT x_tmp___ = (call); if (CurException) return (ResT()); x_tmp___;})
#define TRY_CALL_VOID(ResT, call) ({(call); if (CurException) return (ResT()); void();})

#define TRY_CALL_(CallT, call, action) ({CallT x_tmp___ = (call); if (CurException) {action;} x_tmp___;})
#define TRY_CALL_VOID_(call, action) ({(call); if (CurException) {action;} void();})
#define CHECK_EXCEPTION(action) if (CurException) {action;}

#define TRY_CALL_EXIT(CallT, message, call) ({CallT x_tmp___ = (call); if (CurException) {php_critical_error (message);} x_tmp___;})
#define TRY_CALL_VOID_EXIT(message, call) ({(call); if (CurException) {php_critical_error (message);} void();})
#define FREE_EXCEPTION php_assert (CurException != nullptr); CurException->~Exception(); dl::deallocate (CurException, sizeof (Exception)); CurException = nullptr

Exception f$Exception$$__construct(const string &file, int line, const string &message = string(), int code = 0);

Exception f$err(const string &file, int line, const string &code, const string &desc = string());


string f$Exception$$getMessage(const Exception &e);

int f$Exception$$getCode(const Exception &e);

string f$Exception$$getFile(const Exception &e);

int f$Exception$$getLine(const Exception &e);

array<array<string>> f$Exception$$getTrace(const Exception &e);

string f$Exception$$getTraceAsString(const Exception &e);


bool f$boolval(const Exception &my_exception);

bool eq2(const Exception &my_exception, bool value);

bool eq2(bool value, const Exception &my_exception);

bool equals(bool value, const Exception &my_exception);

bool equals(const Exception &my_exception, bool value);


void free_exception_lib();
