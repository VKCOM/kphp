@kphp_should_fail k2_skip
/kphp-serialized-field: 1 is already in use/
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class A {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $x = 10;

    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $y = 10;
}

$a = new A();
instance_serialize($a);
