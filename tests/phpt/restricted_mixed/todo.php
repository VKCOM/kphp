@ok
<?php

/**
 * we don't propagate mixed union info to vars right now,
 * so using an incorrect default init is not an error yet
 * @param int|string $x
 */
function int_or_string_null_default($x = null) {}

/** @return float|boolean */
function get_float_or_bool() { return 0.5; }

/** @param int|string $x */
function expect_int_or_string($x) {}

/** @param int|string $x */
function mismatching_check($x) {
  // when mixed unions stop accept bare mixed type and are propagated properly,
  // this code should probably result in a compile error (or some warning)
  if (is_array($x)) {
    var_dump('should never happen');
  }
}

function test() {
  int_or_string_null_default();

  // unions as return values are not preserved as types yet
  expect_int_or_string(get_float_or_bool());

  mismatching_check(1);
}

test();
