@ok php7_4
<?php

class Foo {}

function singleton_foo(): Foo {
  static $instance = null;
  if ($instance) {
    return $instance;
  }
  $instance = new Foo();
  return $instance;
}

function singleton_foo2(): Foo {
  static $instance2 = null;
  if ($instance2) {
    return $instance2;
  }
  $instance2 = new Foo();
  return $instance2;
}

singleton_foo();
singleton_foo2();
