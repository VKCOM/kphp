@kphp_should_fail
/assign int to A::\$name/
/but it's declared as @var string/
<?php

class A {
  /** @var string */
  public $name = 0;
}

new A;
