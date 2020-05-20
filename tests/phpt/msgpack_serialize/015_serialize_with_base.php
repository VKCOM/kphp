@ok
<?php

require_once 'polyfills.php';

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

    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $y = 10;
}

$a = new A();
$str = instance_serialize($a);
$a_new = instance_deserialize($str, A::class);
$a_new->foo();
