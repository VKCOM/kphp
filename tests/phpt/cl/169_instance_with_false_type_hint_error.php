@kphp_should_fail
/pass false to argument \$a of demo/
/but it's declared as @param A/
<?php

class A {
  public $x = 1;
}

function demo(A $a) {
  echo !!$a;
}

demo(false);
demo(new A);
