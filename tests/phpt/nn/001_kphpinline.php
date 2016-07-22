@ok
<?php

/** @kphp-inline */

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

?>


