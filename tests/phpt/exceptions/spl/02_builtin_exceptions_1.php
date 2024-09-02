@ok php8.2
KPHP_REQUIRE_FUNCTIONS_TYPING=1
<?php

/*
    Random\RandomException (extends Exception)
*/

function fmt_throwable(Throwable $e): string {
  return get_class($e) . ":{$e->getLine()}:{$e->getMessage()}:{$e->getCode()}";
}

function test_random_exception() {
  $to_throw = new Random\RandomException(__FUNCTION__, __LINE__ + 1);

  try {
    throw $to_throw;
  } catch (LogicException $e) {
    var_dump([__LINE__ => 'unreachable']);
  } catch (RuntimeException $e) {
    var_dump([__LINE__ => 'unreachable']);
  } catch (Exception $e) {
    var_dump([__LINE__ => fmt_throwable($e)]);
    var_dump([__LINE__ => $e->getMessage()]);
  }

  try {
    throw $to_throw;
  } catch (Random\RandomException $e) {
    var_dump([__LINE__ => fmt_throwable($e)]);
    var_dump([__LINE__ => $e->getMessage()]);
  }
}

test_random_exception();
