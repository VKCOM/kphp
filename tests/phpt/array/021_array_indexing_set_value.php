@kphp_should_fail
/but Foo type is passed/
/but int\[\] type is passed/
<?php

class Foo {}

function f() {
  $x = [1];
  $x[new Foo] = 2;
  $x[[1, 2]] = 2;
  $x["1"] = 2;
  $x[2] = 2;
}

f();
