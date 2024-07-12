@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class ExampleObject {
  /** @var bool */
  public $b1 = true;
  /** @var bool */
  public $b2 = false;
  /** @var bool */
  public $b3 = true;
  /** @var bool */
  public $b4 = false;
  /** @var bool */
  public $b5 = true;

  /** @var int */
  public $i1 = 1;
  /** @var int */
  public $i2 = 2;

  /** @var bool */
  public $b6 = false;
  /** @var bool */
  public $b7 = true;

  /** @var ?bool */
  public $opt_b1 = null;
  /** @var ?bool */
  public $opt_b2 = true;

  /** @var ?int */
  public $opt_i1 = 43;
}

function test(ExampleObject $e) {
  $json = JsonEncoder::encode($e);
  $e2 = JsonEncoder::decode($json, 'ExampleObject');
  var_dump($json);
  var_dump(to_array_debug($e));
  var_dump(to_array_debug($e2));
}

$obj = new ExampleObject();
test($obj);
