@kphp_should_fail
/Can't access/
<?php
class Base {
  private const PRIV = 42;
}
class Derived extends Base {
  public function test() {
    echo self::PRIV;
    echo Base::PRIV;
  }
}

$d = new Derived();
$d->test();
