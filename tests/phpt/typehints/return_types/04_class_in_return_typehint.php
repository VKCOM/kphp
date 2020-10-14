@ok
<?php
class Test {
  public function bar() {
  }
}

/**
 * @return Test
 */
function foo(): Test {
  return new Test();
}

$t = foo();
$t->bar();

