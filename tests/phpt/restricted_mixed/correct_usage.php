@ok
<?php

/** @param string $x */
function ensure_string($x) {}

/** @param int|string $x */
function int_or_string($x) {}

/** @param string|float $x */
function string_or_float($x) {}

/** @param boolean|string|int $x */
function bool_or_string_or_int($x) {}

/** @param int|(mixed[]) $x */
function int_or_mixedarray($x) {}

/** @param array|int $x */
function array_or_int($x) {}

/** @param ?(array|string|int) $x */
function nullable_array_or_string_or_int($x) {}

/** @param array|string|null|int $x */
function nullable_array_or_string_or_int2($x) {}

/** @param array|string|false $x */
function array_or_string_or_false($x) {}

/** @param int|string|false|null $x */
function int_or_string_or_false_or_null($x) {}

/** @param int|float|string|array|boolean|null $x */
function full_mixed($x) {}

/** @param int|float|string|boolean|null $x */
function mixed_except_arrays($x) {}

/** @param int|float $x */
function int_or_float($x) {}

/** @param int|float|string $x */
function int_or_float_or_string($x) {}

/** @param int|string|float $x */
function int_or_float_or_string2($x) {}

/** @param float|string $x */
function float_or_string($x) {}

/**
 * mixing any primitive type with mixed gives mixed
 * @param int|mixed $x
 */
function union_with_mixed($x) {}

/** @param int|string $x */
function default_int_or_string($x = 1) {
  var_dump($x);
}

/**
 * the type system is not good enough to handle this,
 * $x type will be inferred as mixed[]
 * @param boolean[]|float[] $x
 */
function typed_array_union($x) {}

/** @param (int|string)[]|(string|boolean)[] $x */
function typed_array_union2($x) {}

/**
 * this will work as expected
 * @param (boolean|float)[] $x
 */
function typed_array_union3($x) {}

/** @param int|string|false|null $x */
function optional_int_or_string($x) {}

class Example {
  /** @var int|string */
  public $int_or_string;

  /** @var int|string */
  public $int_or_string2;

  /** @var ?(string|array) */
  public $nullable_string_or_array;

  /** @var ?(string|array) */
  public $nullable_string_or_array2;
}

function auto_casts1() {
  /** @var string|null $nullable_string_var */
  $nullable_string_var = null;
  /** @var string|null|false $optional_string_var */
  $optional_string_var = false;

  if ($optional_string_var) {
    int_or_string($optional_string_var);
  }
  if (!is_null($nullable_string_var)) {
    int_or_string($nullable_string_var);
  }
  if ($nullable_string_var !== null) {
    int_or_string($nullable_string_var);
  }
}

function auto_casts2() {
  /** @var string|false $s */
  $s = false;
  if ($s === false || $s === '') {
    return;
  }
  int_or_string($s);
}

function main() {
  /** @var mixed $mixed */
  $mixed = 35;

  /** @var string|array $string_or_array_var */
  $string_or_array_var = [];
  /** @var int|boolean $int_or_bool_var */
  $int_or_bool_var = false;
  /** @var int|string $int_or_string_var */
  $int_or_string_var = 'str';
  /** @var string|null $nullable_string_var */
  $nullable_string_var = null;
  /** @var string|null|false $optional_string_var */
  $optional_string_var = false;

  auto_casts1();
  auto_casts2();

  // since local variables can't preserve the restricted mixed property,
  // we allow mixed to be passed as a restricted mixed argument
  // int_or_string($mixed);

  // int_or_string($string_or_array_var);
  // int_or_string($int_or_bool_var);

  int_or_string(1);
  int_or_string('str');

  string_or_float('str');
  string_or_float(2.5);

  bool_or_string_or_int(true);
  bool_or_string_or_int(false);
  bool_or_string_or_int('str');
  bool_or_string_or_int(1);

  int_or_mixedarray(1);
  int_or_mixedarray([1]);
  int_or_mixedarray(['str']);

  array_or_int([1]);
  array_or_int(['str']);
  array_or_int(1);

  nullable_array_or_string_or_int([1]);
  nullable_array_or_string_or_int('str');
  nullable_array_or_string_or_int(143);
  nullable_array_or_string_or_int(null);

  nullable_array_or_string_or_int2([1]);
  nullable_array_or_string_or_int2('str');
  nullable_array_or_string_or_int2(143);
  nullable_array_or_string_or_int2(null);

  array_or_string_or_false(['str']);
  array_or_string_or_false([1]);
  array_or_string_or_false('str');
  array_or_string_or_false(false);

  int_or_string_or_false_or_null(1);
  int_or_string_or_false_or_null('str');
  int_or_string_or_false_or_null(false);
  int_or_string_or_false_or_null(null);

  full_mixed(1);
  full_mixed(1.4);
  full_mixed('str');
  full_mixed([1]);
  full_mixed(['str']);
  full_mixed(true);
  full_mixed(false);
  full_mixed(null);

  mixed_except_arrays(1);
  mixed_except_arrays(1.4);
  mixed_except_arrays('str');
  mixed_except_arrays(true);
  mixed_except_arrays(false);
  mixed_except_arrays(null);

  int_or_float(1);
  int_or_float(1.4);

  int_or_float_or_string(1);
  int_or_float_or_string(1.5);
  int_or_float_or_string('str');

  int_or_float_or_string2(1);
  int_or_float_or_string2(1.5);
  int_or_float_or_string2('str');

  // float type accepts int, since float|int is reduced to float;
  // to make it consistent, float also behaves like int|float
  $int = 1;
  float_or_string(1);
  float_or_string($int);
  float_or_string(1.5);
  float_or_string('str');

  $obj = new Example();

  var_dump($obj->int_or_string); // this will be null, even though the type is int|string
  $obj->int_or_string = 134;
  $obj->int_or_string = 'str';

  $obj->int_or_string2 = $obj->int_or_string;

  $local_int_or_string = $obj->int_or_string;
  // for now, even if $local_int_or_string type is mixed, we allow assignments
  // of mixed to a restricted mixed to decrease the amount of new compile errors;
  // when restricted mixed types are preserved between the assignments,
  // it will be forbidden to assign an incompatible mixed type
  $obj->int_or_string2 = $local_int_or_string;
  $obj->int_or_string2 = $mixed;
  if (is_string($local_int_or_string)) {
    ensure_string($local_int_or_string);
  }

  $obj->nullable_string_or_array = null;
  $obj->nullable_string_or_array = 'str';
  $obj->nullable_string_or_array = [1];
  $obj->nullable_string_or_array = ['str'];
  $obj->nullable_string_or_array = [];

  $obj->nullable_string_or_array2 = null;
  $obj->nullable_string_or_array2 = $obj->nullable_string_or_array;

  default_int_or_string($mixed);
  default_int_or_string(1);
  default_int_or_string('str');
  default_int_or_string($obj->int_or_string);

  union_with_mixed(1);
  union_with_mixed(1.4);
  union_with_mixed('str');
  union_with_mixed([1]);
  union_with_mixed(['str']);
  union_with_mixed(true);
  union_with_mixed(false);
  union_with_mixed(null);

  typed_array_union(['x']);
  typed_array_union([1]);
  typed_array_union([1.6]);
  typed_array_union([true]);
  typed_array_union([]);
  typed_array_union([$mixed]);

  typed_array_union2(['x']);
  typed_array_union2([1]);
  typed_array_union2([false]);
  typed_array_union2([true]);
  typed_array_union2([]);

  typed_array_union3([3.5]);
  typed_array_union3([1]);
  typed_array_union3([]);
  typed_array_union3([true]);

  optional_int_or_string($int_or_string_var);
  optional_int_or_string($nullable_string_var);
  optional_int_or_string($optional_string_var);
  optional_int_or_string(null);
  optional_int_or_string(false);
  optional_int_or_string(1);
  optional_int_or_string('str');
}

main();
