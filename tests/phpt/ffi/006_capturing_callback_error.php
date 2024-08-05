@kphp_should_fail k2_skip
/FFI callbacks should not capture any variables/
<?php

class Example {
  private $x = 10;

  public function test() {
    $cdef = FFI::cdef('
      void f(int (*g) ());
    ');

    $cdef->f(function () {
      return $this->x;
    });
  }
}

$example = new Example();
$example->test();
