@kphp_should_fail
/but int\[\] type is passed/
/but Foo type is passed/
/but \?int\[\] type is passed/
/but Foo type is passed/
<?php

class Foo{}

function optional_foo(): ?Foo {
  return new Foo;
}

function optional_array(): ?int[] {
  return [];
}

function test_invalid_key_literals() {
  $arr_key = [1, 2, 3];
  $obj_key = new Foo;

  $array = [$arr_key => 1];
  $array = [$obj_key => 2];
  $array = [optional_array() => 3];
  $array = [optional_foo() => 4];
}

test_invalid_key_literals();
