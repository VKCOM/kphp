@kphp_should_fail
/string can't be null in KPHP, while it can be in PHP/
/string can't be null in KPHP, while it can be in PHP/
<?php

function test_string_cant_be_null() {
  $arr = ["foo" => "baz"];
  $obj = $arr["bar"];

  var_dump(isset($obj) ? "not null" : "null");
  var_dump($obj !== null ? "not null" : "null");
}

test_string_cant_be_null();
