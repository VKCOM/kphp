@ok
<?php

function ensure_float(float $x) {}

function ensure_string(string $x) {}

const MICROTIME_AS_FLOAT = true;
const MICROTIME_AS_STRING = false;

const SECONDS_IN_MINUTE = 60;
const SECONDS_IN_HOUR = 60 * SECONDS_IN_MINUTE;
const SECONDS_IN_DAY = 24 * SECONDS_IN_HOUR;
const SECONDS_IN_WEEK = 7 * SECONDS_IN_DAY;

// Sun Jan 04 1970 00:00:00 GMT+0000
const SOME_SUNDAY_MIDNIGHT_TS = SECONDS_IN_DAY * 3;

function test() {
  ensure_float(microtime(true));
  ensure_float(microtime(MICROTIME_AS_FLOAT));
  ensure_string(microtime(false));
  ensure_string(microtime(MICROTIME_AS_STRING));
  ensure_string(microtime());

  $float_time = microtime(true);
  ensure_float($float_time);

  $string_time = microtime();
  list($microsec, $sec) = explode(' ', $string_time);
  var_dump((int)(($sec - SOME_SUNDAY_MIDNIGHT_TS) / SECONDS_IN_WEEK));
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
