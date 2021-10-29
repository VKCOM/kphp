<?php

class FooLib {
  /** @var FooLib */
  private static $instance;

  /** @var ffi_scope<foo> */
  private $lib;

  public static function instance(): FooLib {
    if (self::$instance === null) {
      self::$instance = new FooLib();
    }
    return self::$instance;
  }

  /** @return ffi_cdata<foo, struct Foo> */
  public function new_foo() {
    return $this->lib->new('struct Foo');
  }

  private function __construct() {
    $this->lib = FFI::cdef('
      #define FFI_SCOPE "foo"
      struct Foo {
        float f;
        double d;
        bool b;
      };
    ');
  }
}
