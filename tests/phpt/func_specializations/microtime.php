@ok
<?php

function ensure_float(float $x) {}

function ensure_string(string $x) {}

const MICROTIME_AS_FLOAT = true;
const MICROTIME_AS_STRING = false;

function test() {
  ensure_float(microtime(true));
  ensure_float(microtime(MICROTIME_AS_FLOAT));
  ensure_string(microtime(false));
  ensure_string(microtime(MICROTIME_AS_STRING));
  ensure_string(microtime());

  $float_time = microtime(true);
  ensure_float($float_time);

  $string_time = microtime();
  ensure_string($string_time);
  $string_time = microtime(false);
  ensure_string($string_time);

  $as_float = [false, true];
  foreach ($as_float as $arg) {
    $mixed_time = microtime($arg);
    var_dump(gettype($mixed_time));
  }
}

test();
