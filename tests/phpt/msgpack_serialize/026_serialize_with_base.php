@ok
<?php
require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class Base {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $b = 10;
}

/** @kphp-serializable */
class A extends Base {
    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $x = 10;

    /**
     * @kphp-serialized-field 3
     * @var int
     */
    public $y = 10;
}

$a = new A();
instance_serialize($a);
