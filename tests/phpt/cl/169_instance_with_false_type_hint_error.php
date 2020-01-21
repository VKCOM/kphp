@kphp_should_fail
/mix class A with false/
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
