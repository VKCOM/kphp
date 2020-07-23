@ok
<?php

/** @kphp-inline */

/**
 * @kphp-infer
 * @param int $x
 */
function f($x = 1) {
  if ($x == 1) {
    g();
  }
  var_dump("f()");
}

/** @kphp-inline */

function g() {
  f(2);
  var_dump("g()");
}

/** @kphp-inline */
function h() {
  g();
  f();
  var_dump("h()" /** hello, this is not kphpdoc */);
}

function z() {
  static $i = 0;
  $i++;
  var_dump($i);
  h();
  var_dump("z()");
}

z();


/** @kphp-inline */
/**
 * @kphp-infer
 * @param mixed[] $a
 */
function complex_default_arg($a = ["hello", "world"]) {
  var_dump($a);
}

complex_default_arg();
complex_default_arg([1, 2, 3]);

?>


