@ok
<?php

function type_hint_return($x): ?bool {
  if ($x === 1) {
    return true;
  }
  if ($x === 2) {
    return false;
  }
  return null;
}

function type_hint_arg(?bool $x) {
  var_dump($x);
}

/**
 * @kphp-infer
 * @param int $x
 * @return bool|null $x
 */
function kphp_infer_return_bool_null($x) {
  if ($x === 1) {
    return true;
  }
  if ($x === 2) {
    return false;
  }
  return null;
}

/**
 * @kphp-infer
 * @param int $x
 * @return null|bool $x
 */
function kphp_infer_return_null_bool($x) {
  if ($x === 1) {
    return true;
  }
  if ($x === 2) {
    return false;
  }
  return null;
}

/**
 * @kphp-infer
 * @param bool|null $x1
 * @param null|bool $x2
 */
function kphp_infer_args($x1, $x2) {
  var_dump($x1);
  var_dump($x2);
}


function test_type_hint_return() {
  var_dump(type_hint_return(0));
  var_dump(type_hint_return(1));
  var_dump(type_hint_return(2));
}

function test_type_hint_arg() {
  type_hint_arg(false);
  type_hint_arg(true);
  type_hint_arg(null);
}

function test_kphp_infer_return() {
  var_dump(kphp_infer_return_bool_null(0));
  var_dump(kphp_infer_return_bool_null(1));
  var_dump(kphp_infer_return_bool_null(2));
  var_dump(kphp_infer_return_null_bool(0));
  var_dump(kphp_infer_return_null_bool(1));
  var_dump(kphp_infer_return_null_bool(2));
}

function test_kphp_infer_args() {
  kphp_infer_args(true, null);
  kphp_infer_args(false, null);
  kphp_infer_args(null, null);
  kphp_infer_args(false, true);
  kphp_infer_args(true, false);
  kphp_infer_args(false, false);
}

test_type_hint_return();
test_type_hint_arg();
test_kphp_infer_return();
test_kphp_infer_args();
