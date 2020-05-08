@ok
<?php

require_once "polyfills.php";

function int_or_false() {
  return 1 ? 1 : false;
}

function int_or_null() {
  return 1 ? 1 : null;
}

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

f(drop_false(1));
f(drop_null(1));
f(drop_optional(1));

f(drop_false(int_or_false()));
f(drop_null(int_or_null()));
f(drop_optional(int_or_false()));
f(drop_optional(int_or_null()));

g(drop_null(int_or_null_or_false()));
