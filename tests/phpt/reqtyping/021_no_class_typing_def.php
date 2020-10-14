@ok
<?php

// if KPHP_REQUIRE_CLASS_TYPING = 0, default value doesn't lock the type

class A {
  var $a = 0;

  static $cache = false;
}

$a = new A;
$a->a = [1];

A::$cache = [new A];
