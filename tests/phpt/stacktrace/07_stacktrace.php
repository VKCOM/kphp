@kphp_should_fail php7_4
/assign int to A::\$name/
/but it's declared as @var string/
<?php

class A {
  public string $name = 0;
}

new A;
