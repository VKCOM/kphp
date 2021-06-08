@kphp_should_fail
/but Foo type is passed/
/but bool type is passed/
/but bool type is passed/
/but Foo type is passed/
<?php

class Foo {}

function f() {
  $a = new Foo;
  $b = true;
  $x = [$a => 1];
  $x = [$b => 1];
  $x = [1 => 2, 4 => 12, $b => 1];
  $x = [1 => 2, $a => 1, 3 => 23];
}

f();
