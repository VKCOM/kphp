@ok
<?php
require_once 'kphp_tester_include.php';


/** @kphp-json rename_policy=camelCase */
class B {
  public float $qax_qux;
  /** @kphp-json rename=renamed_qux_quz */
  public bool $qux_quz;
}

/** @kphp-json rename_policy=snake_case */
class A extends B {
  public string $fooBar;
  /** @kphp-json rename=renamed_barBaz */
  public float $barBaz;

  function init() {
    $this->fooBar = "value";
    $this->barBaz = 12.34;
    $this->qax_qux = 98.76;
    $this->qux_quz = false;
  }
}

/** @kphp-json rename_policy=camelCase */
class A1 {
  public string $foo_bar_baz;
  /** @kphp-json rename=renamed */
  public float $qux_qax_quz;
}

/** @kphp-json rename_policy=snake_case */
class B1 {
  public string $fooBarBaz;
  /** @kphp-json rename=renamed */
  public float $quxQaxQuz;
}

/** @kphp-json rename_policy=none */
class C1 {
  public string $fooBarBaz;
  public float $qux_qax_quz;
}


function test_camel_case() {
  $json = "{\"fooBarBaz\":\"value\",\"renamed\":12.34}";
  $obj = JsonEncoder::decode($json, "A1");
  var_dump(to_array_debug($obj));
  if ($obj->foo_bar_baz !== 'value' || $obj->qux_qax_quz != 12.34)
    throw new Exception("unexpected !=");
}

function test_snake_case() {
  $json = "{\"foo_bar_baz\":\"value\",\"renamed\":12.34}";
  $obj = JsonEncoder::decode($json, "B1");
  var_dump(to_array_debug($obj));
}

function test_dont_rename() {
  $json = "{\"fooBarBaz\":\"value\",\"qux_qax_quz\":12.34}";
  $obj = JsonEncoder::decode($json, "C1");
  var_dump(to_array_debug($obj));
}

function test_rename_policy_inherit() {
  $obj = new A;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($restored_obj));
  if (to_array_debug($restored_obj) !== to_array_debug($obj))
    throw new Exception("unexpected !=");
}

test_camel_case();
test_snake_case();
test_dont_rename();
test_rename_policy_inherit();
