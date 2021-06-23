@ok php7_4
<?php

class Foo {}

class FooCache {
  public static function get(): Foo {
    return new Foo();
  }
}

class C1 {
  /** @var Foo $instance */
  public static $instance = null;
  public static function get(): Foo {
    if (self::$instance) {
      return self::$instance;
    }
    self::$instance = new Foo();
    return self::$instance;
  }
}

class C2 {
  /** @var Foo $instance */
  public static $instance = null;
  public static function get(): Foo {
    if (!self::$instance) {
      self::$instance = FooCache::get();
    }
    return self::$instance;
  }
}

C1::get();
C2::get();
