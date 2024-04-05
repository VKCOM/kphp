@ok
KPHP_REQUIRE_FUNCTIONS_TYPING=1
<?php

/*
    LogicException (extends Exception)
        BadFunctionCallException
            BadMethodCallException
        DomainException
        InvalidArgumentException
        LengthException
        OutOfRangeException
    RuntimeException (extends Exception)
        OutOfBoundsException
        OverflowException
        RangeException
        UnderflowException
        UnexpectedValueException
    Random\RandomException (extends Exception)
*/

function fmt_throwable(Throwable $e): string {
  return get_class($e) . ":{$e->getLine()}:{$e->getMessage()}:{$e->getCode()}";
}

function test_logic_exception() {
  try {
    throw new BadMethodCallException(__FUNCTION__, __LINE__ + 1);
  } catch (RuntimeException $e) {
    var_dump([__LINE__ => 'unreachable']);
  } catch (Exception $e2) {
    var_dump([__LINE__ => fmt_throwable($e2)]);
    var_dump([__LINE__ => $e2->getMessage()]);
  }

  try {
    throw new BadMethodCallException(__FUNCTION__, __LINE__ + 1);
  } catch (BadFunctionCallException $e) {
    var_dump([__LINE__ => fmt_throwable($e)]);
    var_dump([__LINE__ => $e->getMessage()]);
  }

  try {
    throw new BadMethodCallException(__FUNCTION__, __LINE__ + 1);
  } catch (BadMethodCallException $e) {
    var_dump([__LINE__ => fmt_throwable($e)]);
    var_dump([__LINE__ => $e->getMessage()]);
  }
}

function test_runtime_exception() {
  $to_throw = new OverflowException(__FUNCTION__, __LINE__ + 1);

  try {
    throw $to_throw;
  } catch (LogicException $e) {
    var_dump([__LINE__ => 'unreachable']);
  } catch (Exception $e2) {
    var_dump([__LINE__ => fmt_throwable($e2)]);
    var_dump([__LINE__ => $e2->getMessage()]);
  }

  try {
    throw $to_throw;
  } catch (RuntimeException $e) {
    var_dump([__LINE__ => fmt_throwable($e)]);
    var_dump([__LINE__ => $e->getMessage()]);
  }
}

test_logic_exception();
test_runtime_exception();
