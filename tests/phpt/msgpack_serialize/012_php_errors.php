@ok
<?php
#ifndef KPHP

require_once 'kphp_tester_include.php';

function my_assert($value) {
  if ($value === false) {
    assert(false);
    exit(1);
  }
}

/** @kphp-serializable */
class PrimitiveError {
  /**
   * @kphp-serialized-field 1
   * @var int
   */
  public $i = 2.78;
}

my_assert(instance_serialize(new PrimitiveError()) === null);

/** @kphp-serializable */
class ErrorInside {
  /**
   * @kphp-serialized-field 1
   * @var PrimitiveError[]
   */
  public $b = [];

  public function __construct() {
    $this->b = [new PrimitiveError()];
  }
}

my_assert(instance_serialize(new ErrorInside()) === null);

/** @kphp-serializable */
class TupleError {
  /**
   * @kphp-serialized-field 1
   * @var tuple(int, string)
   */
  public $t;

  public function __construct() {
    $this->t = tuple(10, 20);
  }
}

my_assert(instance_serialize(new TupleError()) === null);

/** @kphp-serializable */
class MixInstancesAndPrimitive {
  /**
   * @kphp-serialized-field 1
   * @var mixed
   */
  public $t;

  public function __construct() {
    $this->t = new \Exception();
  }
}

my_assert(instance_serialize(new MixInstancesAndPrimitive()) === null);

/** @kphp-serializable */
class OrTypeError {
  /**
   * @kphp-serialized-field 1
   * @var int|string
   */
  public $t = null;
}

my_assert(instance_serialize(new OrTypeError()) === null);

/** @kphp-serializable */
class IntOrFalseArray {
  /**
   * @kphp-serialized-field 1
   * @var (int|false)[]
   */
  public $t = [true];
}

my_assert(instance_serialize(new IntOrFalseArray()) === null);

/** @kphp-serializable */
class IntArray {
  /**
   * @kphp-serialized-field 1
   * @var int[]
   */
  public $t = [true];
}

my_assert(instance_serialize(new IntArray()) === null);
#endif

var_dump('OK');
