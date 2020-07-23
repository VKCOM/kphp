@ok
<?php

require_once "polyfills.php";

function f_int(int $x) {
  echo "Hi, i'm int!";
}

function f_double(double $x) {
  echo "Hi, i'm double!";
}

function f_bool(bool $x) {
  echo "Hi, i'm int!";
}

/**
 * @kphp-infer
 * @param int|false $x
 */
function f_int_or_false($x) {
  echo "Hi, i'm int or false!";
  if ($x) {
    f_int($x);
  }
  if (!is_bool($x)) {
    f_int($x);
  }
  if ($x !== false) {
    f_int($x);
  }
  if ($x != false) {
    f_int($x);
  }
  if (!($x == false)) {
    f_int($x);
  }
  if (!($x === false)) {
    f_int($x);
  }
}

/**
 * @kphp-infer
 * @param int|null $x
 */
function f_int_or_null($x) {
  echo "Hi, i'm int or null!";
  if ($x) {
    f_int($x);
  }
  if (isset($x)) {
    f_int($x);
  }
  if (!is_null($x)) {
    f_int($x);
  }
  if ($x !== null) {
    f_int($x);
  }
  if ($x != null) {
    f_int($x);
  }
  if (!($x == null)) {
    f_int($x);
  }
  if (!($x === null)) {
    f_int($x);
  }
}

/**
 * @kphp-infer
 * @param int|false|null $x
 */
function f_int_or_false_or_null($x) {
  echo "Hi, i'm int or null!";
  if ($x) {
    f_int($x);
  }
  if (!is_null($x) && $x !== false) {
    f_int($x);
  }
  if (!is_null($x) && !is_bool($x)) {
    f_int($x);
  }
}


function f_string(string $x) {
  echo "Hi, i'm int!";
}

function f_array(array $x) {
  echo "Hi, i'm array!";
}

/**
 * @kphp-infer
 * @param mixed $x
 */
function f($x) {
  if (is_int($x)) {
    f_int($x);
  } 
  if (is_string($x)) {
    f_string($x);
  } 
  if (is_array($x)) {
    f_array($x);
  } 
  if (is_bool($x)) {
    f_bool($x);
  } 
  if (is_float($x)) {
    f_double($x);
  } 
  if ($x === false) {
    f_int_or_false($x);
  } 
  if ($x === null) {
    f_int_or_null($x);
  }

  f_int(is_int($x) ? $x : 0);
  var_dump($x);
}

f(1);
f("1");
f([1]);

f_int_or_false(1);
f_int_or_false(false);
f_int_or_null(1);
f_int_or_null(null);
f_int_or_false_or_null(1);
f_int_or_false_or_null(null);
f_int_or_false_or_null(false);
