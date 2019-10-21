@kphp_should_fail
/Expected return typehint after/
<?php
class A {
  public static function foo():static {
    return new self();
  }
}

