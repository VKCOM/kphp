@kphp_should_fail
/Magic method A::__toString must have string return type/
<?php

class A {
  function __toString() : ?string {
    return null;
  }
}

function demo(A $a) {
  echo $a;
}

demo(new A);