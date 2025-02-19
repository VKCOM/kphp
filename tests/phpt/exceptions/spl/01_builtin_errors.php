@ok
KPHP_REQUIRE_FUNCTIONS_TYPING=1
<?php

#ifndef KPHP
if (PHP_VERSION_ID < 80000) {
  // ValueError is a PHP8 class
  class ValueError extends Error {}
}
#endif

/*
 Error
    ArithmeticError
        DivisionByZeroError
    AssertionError
    CompileError
        ParseError
    TypeError
        ArgumentCountError
    ValueError
    UnhandledMatchError
 */

function fmt_throwable(Throwable $e): string {
  return get_class($e) . ":{$e->getLine()}:{$e->getMessage()}:{$e->getCode()}";
}

function test_division_by_zero_error() {
  try {
    throw new DivisionByZeroError(__FUNCTION__, __LINE__ + 1);
  } catch (Exception $e) {
    var_dump([__LINE__ => 'unreachable']);
  } catch (Error $err) {
    var_dump([__LINE__ => fmt_throwable($err)]);
    var_dump([__LINE__ => $err->getMessage()]);
  }

  try {
    throw new DivisionByZeroError(__FUNCTION__, __LINE__ + 1);
  } catch (ArithmeticError $err) {
    var_dump([__LINE__ => fmt_throwable($err)]);
    var_dump([__LINE__ => $err->getMessage()]);
  }

  try {
    throw new DivisionByZeroError(__FUNCTION__, __LINE__ + 1);
  } catch (DivisionByZeroError $err) {
    var_dump([__LINE__ => fmt_throwable($err)]);
    var_dump([__LINE__ => $err->getMessage()]);
  }
}

function test_value_error() {
  try {
    throw new ValueError(__FUNCTION__, __LINE__ + 1);
  } catch (Exception $e) {
    var_dump([__LINE__ => 'unreachable']);
  } catch (Error $err) {
    var_dump([__LINE__ => fmt_throwable($err)]);
    var_dump([__LINE__ => $err->getMessage()]);
  }

  try {
    throw new ValueError(__FUNCTION__, __LINE__ + 1);
  } catch (ValueError $err) {
    var_dump([__LINE__ => fmt_throwable($err)]);
    var_dump([__LINE__ => $err->getMessage()]);
  }
}

function test_type_error() {
  try {
    throw new TypeError(__FUNCTION__, __LINE__ + 1);
  } catch (Exception $e) {
    var_dump([__LINE__ => 'unreachable']);
  } catch (Error $err) {
    var_dump([__LINE__ => fmt_throwable($err)]);
    var_dump([__LINE__ => $err->getMessage()]);
  }

  try {
    throw new TypeError(__FUNCTION__, __LINE__ + 1);
  } catch (TypeError $err) {
    var_dump([__LINE__ => fmt_throwable($err)]);
    var_dump([__LINE__ => $err->getMessage()]);
  }
}

test_division_by_zero_error();
test_value_error();
test_type_error();
