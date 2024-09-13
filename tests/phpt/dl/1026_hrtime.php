@ok k2_skip
<?php

// We can't test the exact values, but we can try to
// check some common-sense properties.
// For example, the results should be monotonic.

#ifndef KPHP
// This code tries to add a hrtime polyfill if it's not available.

const NANO_IN_SEC = 1000000000;

$hrtime_base = (int)(microtime(true) * NANO_IN_SEC);

function hrtime_polyfill(bool $as_number = false) {
  $now = (int)(microtime(true) * NANO_IN_SEC);
  $nanoseconds = (int)($now - $hrtime_base);

  if ($as_number) {
    return $nanoseconds;
  }

  $seconds = (int)($nanoseconds / NANO_IN_SEC);
  $nanoseconds -= (int)($seconds % $nanoseconds);
  return [$seconds, $nanoseconds];
}

if (!function_exists('hrtime')) {
  function hrtime($as_number = false) {
    return hrtime_polyfill($as_number);
  }
}
#endif

function validate_hrtime_array(array $t) {
  var_dump(count($t) === 2);
  var_dump(is_int($t[0]));
  var_dump(is_int($t[1]));
}

function test_array() {
  $first = hrtime();
  usleep(5);
  $second = hrtime();

  if (is_array($first)) {
    validate_hrtime_array($first);
  }
  if (is_array($second)) {
    validate_hrtime_array($second);
  }

  var_dump(abs($first[0] - $second[0]) < 3);
  if ($first[0] === $second[0]) {
    var_dump($first[1] < $second[1]);
  } else {
    var_dump($first[0] < $second[0]);
  }
}

function test_number() {
  $first = hrtime();
  usleep(5);
  $second = hrtime();

  var_dump($first < $second);
}

var_dump(hrtime() !== false);
var_dump(hrtime() !== 0);
var_dump(is_int(hrtime()));

test_array();
test_number();
