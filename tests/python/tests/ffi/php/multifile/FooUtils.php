<?php

class FooUtils {
  /** @param ffi_cdata<foo, struct Foo> $foo */
  public static function zero($foo) {
    $foo->d = 0.0;
    $foo->f = 0.0;
    $foo->b = false;
  }

  /** @param ffi_cdata<foo, struct Foo> $foo */
  public static function dump($foo) {
    var_dump([$foo->d, $foo->f, $foo->b]);
  }
}
