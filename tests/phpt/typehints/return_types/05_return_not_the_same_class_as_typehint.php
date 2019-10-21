@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php
class Test {
  public function empty() {}
}
class Zoo {
  public function empty() {}
}

function foo(): Test {
  return new Zoo();
}

$t = foo();
$t->empty();

