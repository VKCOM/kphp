@ok
<?php
require_once 'kphp_tester_include.php';

class Base {
    public function foo() { var_dump("OK"); }
}

/** @kphp-serializable */
class A extends Base {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $x = 10;
}

/** @kphp-serializable */
class B extends A {
    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $y = 10;
}

/** @kphp-serializable */
class C extends B {
    /**
     * @kphp-serialized-field 3
     * @var int
     */
    public $z = 12;
}

$c0 = new C();
$str = instance_serialize($c0);
$c0->foo();

$c = instance_deserialize($str, C::class);
$b = instance_deserialize($str, B::class);
$a = instance_deserialize($str, A::class);

var_dump(instance_to_array($a));
var_dump(instance_to_array($b));
var_dump(instance_to_array($c));

$a->foo();
$b->foo();
$c->foo();
