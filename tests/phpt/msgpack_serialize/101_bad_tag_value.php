@kphp_should_fail k2_skip
/kphp-serialized-field\(-10\) must be >=0 and < than 127/
/kphp-serialized-field\(128\) must be >=0 and < than 127/
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class A {
    /**
     * @kphp-serialized-field -10
     * @var int
     */
    public $x = 10;

    /**
     * @kphp-serialized-field 128
     * @var int
     */
    public $y = 10;
}

$a = new A();
instance_serialize($a);
