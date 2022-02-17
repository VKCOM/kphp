@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Base::f\(\)/
<?php

abstract class Base {
  /**
   * @param $cb callable(int):void
   */
  public abstract function f(callable $cb);
}

class Impl extends Base {
  /**
   * @param $cb callable(string):void
   */
  public function f(callable $cb) {}
}

