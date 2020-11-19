@kphp_should_fail
/Invalid property access ...->x: does not exist in class `static lambda`/
<?php

class Example {
  public function test_static_bad($x) {
    $f = static fn() => $this->x;
    var_dump($f());
  }

  private $x = 111;
}

$obj = new Example();
$obj->test_static_bad();
