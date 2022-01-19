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

function test_invalid_indexing() {
  $array = [1, 2, 3];
  echo $array[$array];
  echo $array[new Foo];
  echo $array[optional_array()];
  echo $array[optional_foo()];
}

test_invalid_indexing();
