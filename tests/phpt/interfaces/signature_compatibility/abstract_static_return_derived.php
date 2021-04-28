@ok php7_4
<?php

class A {}
class B extends A {}

abstract class Base {
  public abstract static function f(): A;
}

class Impl extends Base {
  public static function f(): B { return new B(); }
}

/** @var Base[] $list */
$list = [new Impl()];
