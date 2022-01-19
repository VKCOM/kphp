@kphp_should_fail
/but int\[\] type is passed/
/but Foo type is passed/
/but string type is passed/
/but float type is passed/
/but \?int\[\] type is passed/
/but Foo type is passed/
/but \?string type is passed/
/but \?float type is passed/
<?php

class Foo {}

function optional_foo(): ?Foo {
  return new Foo;
}

function optional_array(): ?int[] {
  return [];
}

function optional_string(): ?string {
  return "key";
}

function optional_float(): ?float {
  return 1.5;
}

function test_invalid_indexing() {
  $array = [1, 2, 3];
  $string = "Hello";

  echo $string[$array];
  echo $string[new Foo];
  echo $string["key"];
  echo $string[1.5];

  echo $string[optional_array()];
  echo $string[optional_foo()];
  echo $string[optional_string()];
  echo $string[optional_float()];
}

test_invalid_indexing();
