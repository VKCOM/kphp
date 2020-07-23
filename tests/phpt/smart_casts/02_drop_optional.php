@ok
<?php

require_once "polyfills.php";

/**
 * @kphp-infer
 * @return int|false
 */
function int_or_false() {
  return 1 ? 1 : false;
}

/**
 * @kphp-infer
 * @return int|null
 */
function int_or_null() {
  return 1 ? 1 : null;
}

/**
 * @kphp-infer
 * @return int|null|false
 */
function int_or_null_or_false() {
  return 1 ? 1 : (1 ? false : null);
}

function f(int $x): int {
  return $x;
}

/**
 * @kphp-infer
 * @param int|false $x
 * @return int|false
 */
function g($x) {
  return $x;
}

f(not_false(1));
f(not_null(1));
f(not_false(not_null(1)));

f(not_false(int_or_false()));
f(not_null(int_or_null()));
f(not_false(not_null(int_or_false())));
f(not_false(not_null(int_or_null())));

g(not_null(int_or_null_or_false()));
