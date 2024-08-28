@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/**
 * @kphp-serializable
 **/
class ExampleObject {
  /**
   * @kphp-serialized-field 1
   * @var bool
   */
  public $b1 = true;
  /**
   * @kphp-serialized-field 2
   * @var bool
   */
  public $b2 = false;
  /**
   * @kphp-serialized-field 3
   * @var bool
   */
  public $b3 = true;
  /**
   * @kphp-serialized-field 4
   * @var bool
   */
  public $b4 = false;
  /**
   * @kphp-serialized-field 5
   * @var bool
   */
  public $b5 = true;

  /**
   * @kphp-serialized-field 6
   * @var int
   */
  public $i1 = 1;
  /**
   * @kphp-serialized-field 7
   * @var int
   */
  public $i2 = 2;

  /**
   * @kphp-serialized-field 8
   * @var bool
   */
  public $b6 = false;
  /**
   * @kphp-serialized-field 9
   * @var bool
   */
  public $b7 = true;

  /**
   * @kphp-serialized-field 10
   * @var ?bool
   */
  public $opt_b1 = null;
  /**
   * @kphp-serialized-field 11
   * @var ?bool
   */
  public $opt_b2 = true;

  /**
   * @kphp-serialized-field 12
   * @var ?int
   */
  public $opt_i1 = 43;
}

function test() {
    $obj = new ExampleObject();
    $serialized = instance_serialize($obj);
    var_dump(base64_encode($serialized));
    $deserialized = instance_deserialize($serialized, ExampleObject::class);
    var_dump(to_array_debug($deserialized));
    var_dump($deserialized->b1);
    var_dump($deserialized->b2);
    var_dump($deserialized->opt_i1);
    var_dump($deserialized->opt_b1);
    var_dump($deserialized->opt_b1);
}

test();
