@ok
<?php

class A {
}

class B {
  /** @var B[] */
  public static $b = [];

  /**
   * @kphp-infer
   * @return bool
   */
  public static function foo() {
    return isset(self::$b[1]);
  }
}

class C {
}

class D {
  /** @var C */
  public $c = null;
}

function test_new_fully_static_class() {
  $a = new A;
  var_dump(get_class($a));
}

function test_use_fully_static_class() {
  var_dump(B::foo());
}

function test_fully_static_class_as_field() {
  $d = new D;
  var_dump(is_null($d->c));
}

test_new_fully_static_class();
test_use_fully_static_class();
test_fully_static_class_as_field();
