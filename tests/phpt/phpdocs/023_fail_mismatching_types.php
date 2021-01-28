@kphp_should_fail
/023_fail_mismatching_types.php:8/
/4192 is int/
<?php

class Foo {
  /** @var string */
  public $s = 4192;
}

$foo = new Foo();
