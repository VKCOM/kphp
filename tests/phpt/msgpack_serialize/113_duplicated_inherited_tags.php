@kphp_should_fail k2_skip
/kphp-serialized-field: field with number 1 found in both classes A and B/
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class A {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $x = 10;

}

/** @kphp-serializable */
class B extends A {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $y = 10;
}

$b = new B();
instance_serialize($b);
