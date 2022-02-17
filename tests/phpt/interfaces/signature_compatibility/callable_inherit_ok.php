@ok
<?php

abstract class Base {
  /**
   * @param $cb callable(int):void
   */
  public abstract function f(callable $cb);
}

class Impl extends Base {
  /**
   * @param $cb callable(int):void
   */
  public function f(callable $cb) {}
}

