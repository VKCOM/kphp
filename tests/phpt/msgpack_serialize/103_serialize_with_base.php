@kphp_should_fail
/You may not serialize polymorphic classes/
<?php

require_once 'polyfills.php';

class Base {
    public $b = 10;
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
instance_serialize($a);
