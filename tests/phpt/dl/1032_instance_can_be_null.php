@ok
<?php

class A {
  public $x = 1;
}

function test_instance_can_be_null() {
  $arr = ["foo" => new A];
  $obj = $arr["bar"];

  var_dump(isset($obj) ? "not null" : "null");
  var_dump($obj !== null ? "not null" : "null");
}

test_instance_can_be_null();
