@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class B {
  /** @kphp-json rename=new_b_foo */
  public string $b_foo;
}

class C {
  /** @kphp-json rename=new_c_bar */
  public float $c_bar;
  // no tag
  public bool $c_baz;
}

class A extends C {
  // no tag
  public string $foo;
  /** @kphp-json rename=bar */
  public float $bar;
  /** @kphp-json rename=new_baz */
  public bool $baz;
  /** @kphp-json rename=new_b */
  public B $b;

  function init() {
    $this->foo = "value2";
    $this->bar = 12.34;
    $this->baz = true;
    $this->b = new B;
    $this->b->b_foo = "value1";
    $this->c_bar = 98.76;
    $this->c_baz = false;
  }
}

class AOld {
  /** @kphp-json rename=foo_json */
  public string $foo;
  /** @kphp-json rename=bar_json */
  public string $bar;

  function init() {
    $this->foo = "foo_value";
    $this->bar = "bar_value";
  }
}

class ANew {
  /** @kphp-json rename=foo_json */
  public string $foo_renamed;
  /** @kphp-json rename=bar_json */
  public string $bar_renamed;
}


function test_override_json_keys() {
  $obj = new A;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  if (strpos($json, 'new_b_foo') === false) throw new Exception("unexpected not found");
  $restored_obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($restored_obj));
}

function test_variable_renaming() {
  $obj = new AOld;
  $obj->init();
  $json = JsonEncoder::encode($obj);

  $obj_restored = JsonEncoder::decode($json, "ANew");
  var_dump(to_array_debug($obj_restored));
}

test_override_json_keys();
test_variable_renaming();
