<?php

/*
 * This file is included in every KPHP .php test (KPHP tests are: run php, compile and run kphp, compare results).
 * It contais autoload for PHP tests and kphp polyfills.
 * Do not use it in your PHP project.
 */

#ifndef KPHP

if (getenv('KPHP_TESTS_POLYFILLS_REPO') === false) {
  die('$KPHP_TESTS_POLYFILLS_REPO is unset');
}
(function ($class_loader) {
  // add local classes that are placed near the running test
  [$script_path] = get_included_files();
  $class_loader->addPsr4("", dirname($script_path));
})(require_once getenv('KPHP_TESTS_POLYFILLS_REPO', true) . '/vendor/autoload.php');

define('kphp', 0);

if (false)
#endif
define('kphp', 1);

function die_if_failure(bool $is_ok, string $msg) {
  if (!$is_ok) {
    $failure_msg = "Failure\n$msg\n";
#ifndef KPHP
    $failure_msg .= (new \Exception)->getTraceAsString();
#endif
    warning($failure_msg);
    die(1);
  }
}

function assert_int_eq3(int $lhs, int $rhs) {
  echo "assert_int_eq3($lhs, $rhs)\n";
  die_if_failure($lhs === $rhs, "Expected equality of these values: $lhs and $rhs");
}

/**
 * @param int[]|null|false $lhs - always expect array, trick kphp to avoid type error
 * @param int[] $rhs
 */
function assert_array_int_eq3($lhs, array $rhs) {
  $lhs_str = print_r($lhs, true);
  $rhs_str = print_r($rhs, true);
  echo "assert_array_int_eq3($lhs_str, $rhs_str)\n";
  die_if_failure($lhs === $rhs, "Expected equality of these values:\n$lhs_str\nand\n$rhs_str");
}

function assert_str_eq3(string $lhs, string $rhs) {
  echo "assert_str_eq3($lhs, $rhs)\n";
  die_if_failure($lhs === $rhs, "Expected equality of these values: $lhs and $rhs");
}

function assert_false(bool $value) {
  echo "assert_false($value)\n";
  die_if_failure(!$value, "Expected false");
}

function assert_true(bool $value) {
  echo "assert_true($value)\n";
  die_if_failure($value, "Expected true");
}

function assert_near(float $lhs, float $rhs, float $abs_error = 1e-6) {
  echo "assert_near($lhs, $rhs, $abs_error)\n";
  $diff = abs($lhs - $rhs);
  die_if_failure(abs($lhs - $rhs) <= $abs_error,
                 "The difference between $lhs and $rhs is $diff, which exceeds $abs_error");
}
