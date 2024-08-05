@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class A {
  public $instance_field = "foo" . " " . "bar";
  public static $static_field = "foo" . " " . "bar";
}

class B {
  public $instance_field = "foo" . " " . "bar" . " " . "baz" . "foo";
  public static $static_field = "foo" . " " . "bar" . " " . "baz" . "foo";
}

const c = "bar";

class C {
  public $instance_field = c . " " . "bar";
  public static $static_field = c . " " . "bar";
}

$a = new A();
$b = new B();
$c = new C();

echo $a->instance_field, "\n";
echo A::$static_field, "\n";

echo $b->instance_field, "\n";
echo B::$static_field, "\n";

echo $c->instance_field, "\n";
echo C::$static_field, "\n";
