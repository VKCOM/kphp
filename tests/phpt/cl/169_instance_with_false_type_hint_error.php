@kphp_should_fail
/TYPE INFERENCE ERROR/
/Expected type:.+A/
/Actual type:.+false/
<?php

class A {
  public $x = 1;
}

function demo(A $a) {
  echo !!$a;
}

demo(false);
demo(new A);
