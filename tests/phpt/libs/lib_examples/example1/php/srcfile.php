<?php

echo "[example1] srcfile: global run\n";

/**
 * @kphp-lib-export
 * @param string $s
 * @return string
 */
function foo($s) {
  echo "[example1] srcfile: called foo('", $s, "')\n";
  return  "$s == $s";
}

/**
 * @kphp-lib-export
 * @return int
 */
function use_class_static_function() {
  echo "[example1] srcfile: called use_class_static_function()\n";
  echo "[example1] srcfile: call LibClasses\A::static_fun(): \n";
  echo LibClasses\A::static_fun(), "\n";
  return 0;
}

/**
 * @kphp-lib-export
 * @param int $x
 * @return int|false
 */
function return_int_or_false($x) {
  return $x ? $x : false;
}

/**
 * @kphp-lib-export
 * @param int $x
 * @return int|null
 */
function return_int_or_null($x) {
  return $x ? $x : null;
}

/**
 * @kphp-lib-export
 * @param int $x
 * @return false|null
 */
function return_false_or_null($x) {
  return $x ? false : null;
}

/**
 * @kphp-lib-export
 * @param int $x
 * @return int|false|null
 */
function return_int_or_false_or_null($x) {
  if ($x % 3) {
    return false;
  }
  if ($x) {
    return $x;
  }
  return null;
}

/**
 * @kphp-lib-export
 * @param int $x
 * @return false
 */
function return_false($x) {
  return false;
}

/**
 * @kphp-lib-export
 * @param int $x
 * @return null
 */
function return_null($x) {
  return null;
}
