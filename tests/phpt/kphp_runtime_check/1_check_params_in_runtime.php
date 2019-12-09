@ok
<?

require_once 'dl/polyfill/fork-php-polyfill.php';

$warning_msgs = [];

#ifndef KittenPHP
if(0)
#endif
register_kphp_on_warning_callback(function ($message, $bt) {
  global $warning_msgs;
  $warning_msgs[] = $message;
});

function check_warnings($expected) {
#ifndef KittenPHP
  var_dump($expected);
  return;
#endif
  global $warning_msgs;
  var_dump($warning_msgs);
  $warning_msgs = [];
}

/**
 * @kphp-runtime-check
 * @param $x1 int
 */
function test_int_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 string
 */
function test_string_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 double
 */
function test_double_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 bool
 */
function test_bool_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 null
 */
function test_null_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 false
 */
function test_false_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 int|null
 */
function test_int_or_null_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 string|null
 */
function test_string_or_false_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 array|null|false
 */
function test_array_or_false_or_null_param($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 array|null|false
 * @param $x2 int
 * @param $x3 string|false
 * @param $x4 double
 */
function test_several_params($x1, $x2, $x3, $x4) { var_dump($x1); var_dump($x2); var_dump($x3); var_dump($x4); }

/**
 * @kphp-runtime-check
 * @param $x1 string
 */
function test_that_non_var_param_is_not_checked($x1) { var_dump($x1); }

/**
 * @kphp-runtime-check
 * @param $x1 string
 */
function test_resumable_string_param($x1) { sched_yield(); var_dump($x1); }

test_int_param("2");
test_int_param(2);
test_int_param(null);
check_warnings([
  "Got unexpected type [string] of the arg #0 (\$x1) at function test_int_param",
  "Got unexpected type [NULL] of the arg #0 (\$x1) at function test_int_param"
]);

test_string_param("2");
test_string_param(2.01);
test_string_param(true);
check_warnings([
  "Got unexpected type [double] of the arg #0 (\$x1) at function test_string_param",
  "Got unexpected type [bool(true)] of the arg #0 (\$x1) at function test_string_param"
]);

test_double_param(["2"]);
test_double_param(false);
test_double_param(2.01);
check_warnings([
  "Got unexpected type [array] of the arg #0 (\$x1) at function test_double_param",
  "Got unexpected type [bool(false)] of the arg #0 (\$x1) at function test_double_param"
]);

test_bool_param(["2"]);
test_bool_param(true);
check_warnings(["Got unexpected type [array] of the arg #0 (\$x1) at function test_bool_param"]);

test_null_param(["2"]);
test_null_param("123");
test_null_param(null);
check_warnings([
  "Got unexpected type [array] of the arg #0 (\$x1) at function test_null_param",
  "Got unexpected type [string] of the arg #0 (\$x1) at function test_null_param"
]);

test_false_param(["2"]);
test_false_param("123");
test_false_param(false);
check_warnings([
  "Got unexpected type [array] of the arg #0 (\$x1) at function test_false_param",
  "Got unexpected type [string] of the arg #0 (\$x1) at function test_false_param"
]);

test_int_or_null_param(12);
test_int_or_null_param("hhh");
test_int_or_null_param(null);
check_warnings(["Got unexpected type [string] of the arg #0 (\$x1) at function test_int_or_null_param"]);

test_string_or_false_param([12]);
test_string_or_false_param("hhh");
test_string_or_false_param(null);
check_warnings(["Got unexpected type [array] of the arg #0 (\$x1) at function test_string_or_false_param"]);

test_array_or_false_or_null_param([2.1]);
test_array_or_false_or_null_param("hhh");
test_array_or_false_or_null_param(null);
test_array_or_false_or_null_param(false);
check_warnings(["Got unexpected type [string] of the arg #0 (\$x1) at function test_array_or_false_or_null_param"]);

test_several_params([1], 2, false, 4.5);
test_several_params(2, "ff", [null], "ss");
check_warnings([
  "Got unexpected type [integer] of the arg #0 (\$x1) at function test_several_params",
  "Got unexpected type [string] of the arg #1 (\$x2) at function test_several_params",
  "Got unexpected type [array] of the arg #2 (\$x3) at function test_several_params",
  "Got unexpected type [string] of the arg #3 (\$x4) at function test_several_params"
]);
test_several_params(["as"], 2.3, "str", 2);
check_warnings([
  "Got unexpected type [double] of the arg #1 (\$x2) at function test_several_params",
  "Got unexpected type [integer] of the arg #3 (\$x4) at function test_several_params"
]);

test_that_non_var_param_is_not_checked([1, 2, 3]);
test_that_non_var_param_is_not_checked(false);
test_that_non_var_param_is_not_checked(null);
check_warnings([]);

test_resumable_string_param("hello");
wait(fork(test_resumable_string_param(123)));
test_resumable_string_param([55.2]);
check_warnings([
  "Got unexpected type [integer] of the arg #0 (\$x1) at function test_resumable_string_param",
  "Got unexpected type [array] of the arg #0 (\$x1) at function test_resumable_string_param"
]);
