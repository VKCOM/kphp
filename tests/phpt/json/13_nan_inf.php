@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
  public float $nan_f;
  public float $inf_f;
  /** @var mixed */
  public $plus_inf_f;
  public ?float $minus_inf_f;

  function init() {
    $this->nan_f = NAN;
    $this->inf_f = INF;
    $this->plus_inf_f = +INF;
    $this->minus_inf_f = -INF;
  }
}

function test_nan_inf() {
  $obj = new A;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  echo $json, "\n";
  $restored_obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($restored_obj));
}

test_nan_inf();
