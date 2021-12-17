@kphp_should_fail
/field y is not static in klass Example/
<?php

class Example {
  public function test_self_bad($x) {
    $f = static fn() => $x * self::$y;
    var_dump($f());
  }

  private $y = 111;
}

$obj = new Example();
$obj->test_self_bad(10);
