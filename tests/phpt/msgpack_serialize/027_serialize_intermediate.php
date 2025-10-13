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

class B extends A {
}

/** @kphp-serializable */
class C extends B {
    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $y = 12;
}

$c = new C();
$str = instance_serialize($c);
$c->foo();
$c_new = instance_deserialize($str, C::class);
$c_new->foo();
