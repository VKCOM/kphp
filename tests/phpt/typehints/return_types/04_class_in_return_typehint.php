@ok
<?php
class Test {
  public function bar() {
  }
}

function foo(): Test {
  return new Test();
}

$t = foo();
$t->bar();

